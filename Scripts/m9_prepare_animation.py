"""
Builds the operative's locomotion Animation Blueprint from the Dynamic
Locomotion pack.

Run headless:

    UnrealEditor-Cmd.exe <project> -run=pythonscript \
        -script="C:/Coding/executive-ops/Scripts/m9_prepare_animation.py"

Why this exists rather than a hand-built graph: the pack already ships the
locomotion state machine this project needs - start, blend, foot-phased stops,
jump, and both stationary and moving landings - with sync markers already
authored on every clip. Rebuilding that by hand would be worse and slower.

Unreal's Python API cannot create AnimGraph nodes, but it can read and write the
properties of existing ones. So this duplicates the pack's graph into the
project's own folder and repoints it, rather than authoring one from scratch.

What it does:

  1. Marks the two UE4 mannequin skeletons compatible, so the pack's clips and
     graph run on the operative's mesh without retargeting. Both are the same
     rig; they are only separate Skeleton assets because they arrived in
     separate packs.
  2. Duplicates the pack's blend space and Animation Blueprint into
     /Game/Animation as project-owned assets.
  3. Optionally repoints every clip from the in-place set to the root-motion set
     (-rootmotion), and turns root motion on for those clips.
  4. Assigns the result to BP_Operative and switches the mesh from single-node
     playback to the Animation Blueprint.

Idempotent: re-running rebuilds the duplicates from the pack.
"""
import sys

import unreal

EAL = unreal.EditorAssetLibrary

TAG = "Anim"

PACK = "/Game/DynamicLocomotion/Animations"
PACK_SKELETON = "/Game/DynamicLocomotion/Characters/Mannequin/Mesh/UE4_Mannequin_Skeleton"
OUR_SKELETON = "/Game/OpenWorldAnimset/UE4_Mannequin/Mesh/UE4_Mannequin_Skeleton"

DEST = "/Game/Animation"
BS_DEST = DEST + "/BS_Locomotion"
ABP_DEST = DEST + "/ABP_Operative"

BP_OPERATIVE = "/Game/Blueprints/BP_Operative"

# Root motion is opt-in: pass -rootmotion.
#
# The pack ships every clip twice, in-place and root-motion. In-place plus the
# movement component's own velocity is what most shipped third-person games do,
# and with the blend space axis calibrated to the speeds the clips were authored
# at (180 / 500 / 950 cm/s) the feet do not slide. Root motion is the better
# answer for authored one-shots - vaults, takedowns, landings - where the clip
# should dictate the distance exactly.
USE_ROOT_MOTION = "-rootmotion" in sys.argv


def log(msg):
    unreal.log_warning("[{}] {}".format(TAG, msg))


def make_skeletons_compatible():
    """
    Tell the engine the two skeletons are the same rig.

    This is the supported alternative to duplicating and retargeting every clip:
    an animation authored against one skeleton plays on a mesh using the other.
    Marked in both directions so it does not matter which asset an animation,
    montage or Animation Blueprint happens to reference.
    """
    ours = unreal.load_asset(OUR_SKELETON)
    theirs = unreal.load_asset(PACK_SKELETON)

    if not ours or not theirs:
        raise RuntimeError("skeletons not found - is the Dynamic Locomotion pack imported? "
                           "Run Scripts/import_fab_assets.ps1")

    ours.add_compatible_skeleton(theirs)
    theirs.add_compatible_skeleton(ours)

    EAL.save_loaded_asset(ours)
    EAL.save_loaded_asset(theirs)
    log("skeletons marked compatible in both directions")


def root_motion_clip(in_place_clip):
    """The _RM twin of an _IP clip, if the pack ships one."""
    name = in_place_clip.get_name()
    if not name.endswith("_IP"):
        return None

    candidate = "{}/RootMotion/{}_RM".format(PACK, name[:-3])
    return unreal.load_asset(candidate)


def enable_root_motion(clip):
    """
    Turn root motion on for a clip the graph will actually consume.

    The pack ships the root-motion clips with the flag off, because in-place is
    the pack's default configuration.
    """
    changed = False
    for prop, value in (("enable_root_motion", True),
                        ("force_root_lock", False)):
        if clip.get_editor_property(prop) != value:
            clip.set_editor_property(prop, value)
            changed = True

    if changed:
        EAL.save_loaded_asset(clip)
    return changed


def detach_from_operative():
    """
    Put BP_Operative back on single-node playback before rebuilding.

    Deleting an asset the Blueprint still points at fails, and the rebuild then
    dies on the second run rather than the first - so the reference is dropped
    first and restored at the end.
    """
    blueprint = unreal.load_asset(BP_OPERATIVE)
    if not blueprint:
        return

    cdo = unreal.get_default_object(blueprint.generated_class())
    mesh = cdo.get_editor_property("Mesh")

    if mesh.get_editor_property("anim_class") is None:
        return

    mesh.set_editor_property("anim_class", None)
    mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    EAL.save_loaded_asset(blueprint)
    log("detached the previous Animation Blueprint from BP_Operative")


def clean_previous():
    """Delete in dependency order: the graph references the blend space."""
    for path in (ABP_DEST, BS_DEST):
        if EAL.does_asset_exist(path):
            if not EAL.delete_asset(path):
                raise RuntimeError("could not delete {} - close the editor and retry".format(path))
            log("removed previous " + path)


def duplicate(src, dst):
    asset = EAL.duplicate_asset(src, dst)
    if not asset:
        raise RuntimeError("could not duplicate {} -> {}".format(src, dst))

    # Saved immediately. A duplicate that only exists in memory is written out
    # by whatever saves next - and anything referencing it in the meantime gets
    # serialised with a dangling pointer, which shows up later as "references an
    # unknown Blend Space" and a graph that compiles to nothing.
    EAL.save_loaded_asset(asset)

    log("duplicated {} -> {}".format(src.rsplit("/", 1)[-1], dst))
    return asset


def build_blend_space():
    """
    The project's own copy of the pack's walk/jog/run blend space.

    Sample positions are left exactly as the pack authored them: X is lean
    (-1..1) and Y is speed in cm/s, with samples at 180, 500 and 950. Those
    numbers are the speeds the clips were actually captured at, which is what
    stops the feet sliding - so the character's movement speeds are matched to
    the blend space rather than the other way round.
    """
    blend_space = duplicate(PACK + "/InPlace/WalkJogRun", BS_DEST)

    if not USE_ROOT_MOTION:
        return blend_space

    samples = blend_space.get_editor_property("sample_data")
    swapped = 0

    for sample in samples:
        clip = sample.get_editor_property("animation")
        replacement = root_motion_clip(clip)
        if replacement:
            sample.set_editor_property("animation", replacement)
            enable_root_motion(replacement)
            swapped += 1

    blend_space.set_editor_property("sample_data", samples)
    EAL.save_loaded_asset(blend_space)

    log("blend space: {}/{} samples repointed to root motion".format(swapped, len(samples)))
    return blend_space


def build_anim_blueprint(blend_space):
    """
    The project's own copy of the pack's locomotion graph, repointed at the
    project's assets.

    Node properties are writable from Python even though nodes cannot be
    created, so every sequence player and the blend space player are redirected
    here rather than by hand in the editor.
    """
    abp = duplicate(PACK + "/ThirdPerson_AnimBP", ABP_DEST)

    # Blend space players first: there is only one in the pack's graph, driving
    # the ground locomotion state.
    for node in abp.get_nodes_of_class(unreal.AnimGraphNode_BlendSpacePlayer):
        inner = node.get_editor_property("node")
        inner.set_editor_property("blend_space", blend_space)
        node.set_editor_property("node", inner)

    log("blend space player repointed to " + BS_DEST)

    if USE_ROOT_MOTION:
        swapped = 0
        for node in abp.get_nodes_of_class(unreal.AnimGraphNode_SequencePlayer):
            inner = node.get_editor_property("node")
            clip = inner.get_editor_property("sequence")
            if not clip:
                continue

            replacement = root_motion_clip(clip)
            if replacement:
                inner.set_editor_property("sequence", replacement)
                node.set_editor_property("node", inner)
                enable_root_motion(replacement)
                swapped += 1

        log("{} sequence players repointed to root motion".format(swapped))

    unreal.BlueprintEditorLibrary.compile_blueprint(abp)
    EAL.save_loaded_asset(abp)
    return abp


def report_graph(abp):
    """What the duplicated graph actually contains, so the result is checkable."""
    for node in abp.get_nodes_of_class(unreal.AnimGraphNode_SequencePlayer):
        inner = node.get_editor_property("node")
        clip = inner.get_editor_property("sequence")
        if clip:
            log("  plays {} (loop={})".format(
                clip.get_name(), inner.get_editor_property("loop_animation")))


def wire_to_operative(abp):
    blueprint = unreal.load_asset(BP_OPERATIVE)
    if not blueprint:
        raise RuntimeError("BP_Operative not found; run m0_setup.py first")

    cdo = unreal.get_default_object(blueprint.generated_class())
    mesh = cdo.get_editor_property("Mesh")

    mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    mesh.set_editor_property("anim_class", abp.generated_class())

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    EAL.save_loaded_asset(blueprint)

    log("BP_Operative switched to Animation Blueprint mode, using " + ABP_DEST)


def main():
    log("preparing locomotion animation (root motion: {})".format(USE_ROOT_MOTION))

    make_skeletons_compatible()
    detach_from_operative()
    clean_previous()
    blend_space = build_blend_space()
    abp = build_anim_blueprint(blend_space)
    report_graph(abp)
    wire_to_operative(abp)

    log("done")


main()
