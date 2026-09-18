"""
Builds the operative's one-shot montages and assigns them to BP_Operative.

Run headless:

    UnrealEditor-Cmd.exe <project> -run=pythonscript \
        -script="C:/Coding/executive-ops/Scripts/m9_build_montages.py"

Locomotion is a state machine; actions are montages played into the graph's
DefaultSlot. That split is the whole reason the graph needed a Slot node: the
legs keep running the locomotion states underneath while a montage blends the
action in over the top, instead of a clip replacing the entire body.

Run after Scripts/m9_prepare_animation.py, which builds the graph itself.
Idempotent: montages are rebuilt from their source clips each time.
"""
import unreal

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()

TAG = "Montage"

DEST = "/Game/Animation/Montages"
ABP = "/Game/Animation/ABP_Operative"
BP_OPERATIVE = "/Game/Blueprints/BP_Operative"

OWA = "/Game/OpenWorldAnimset/Animations"

# property on the target -> (montage name, source clip)
#
# Blend times are deliberately short. These are reactions, not performances: a
# long blend on a pistol shot reads as the operative thinking about it.
CHARACTER_MONTAGES = {
    "FireMontage":     ("AM_Pistol_Fire",  OWA + "/Pistol/Pistol_shoot_01",   0.05, 0.15),
    "TakedownMontage": ("AM_Takedown",     OWA + "/NPC/Anim_TA_ANG_hit_fist", 0.10, 0.25),
    "DeathMontage":    ("AM_Death",        OWA + "/Deaths/death_aim_chest_01", 0.15, 0.30),
}

TRAVERSAL_MONTAGES = {
    "VaultMontage":  ("AM_Vault",  OWA + "/Vault/Vault_jog",              0.08, 0.20),
    "MantleMontage": ("AM_Mantle", OWA + "/Ledge/High_Ledge_Up_Crouch",   0.10, 0.25),
    "ClimbMontage":  ("AM_Climb",  OWA + "/Climb/Climb_scrambling_path",  0.10, 0.25),
}


def log(msg):
    unreal.log_warning("[{}] {}".format(TAG, msg))


def verify_slot():
    """
    Confirm the graph has the slot these montages play into.

    Without it a montage plays to nothing: Montage_Play reports success, the
    slot's pose is never sampled, and the action silently does not appear.
    """
    abp = unreal.load_asset(ABP)
    if not abp:
        raise RuntimeError("no Animation Blueprint at " + ABP +
                           " - run Scripts/m9_prepare_animation.py first")

    slots = abp.get_nodes_of_class(unreal.AnimGraphNode_Slot)
    if not slots:
        raise RuntimeError(
            "ABP_Operative has no Slot node. Add one in the AnimGraph: state "
            "machine -> Slot 'DefaultSlot' -> Output Animation Pose. "
            "See Docs/Animation.md.")

    names = []
    for node in slots:
        inner = node.get_editor_property("node")
        names.append(str(inner.get_editor_property("slot_name")))

    log("graph slots found: " + ", ".join(names))
    return names


def build_montage(name, clip_path, blend_in, blend_out, slot_name):
    source = unreal.load_asset(clip_path)
    if not source:
        log("SKIP {} - source clip missing: {}".format(name, clip_path))
        return None

    path = DEST + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", source.get_editor_property("skeleton"))

    montage = AT.create_asset(name, DEST, unreal.AnimMontage, factory)
    if not montage:
        log("FAILED to create " + path)
        return None

    # One segment, the whole clip, in the slot the graph exposes.
    #
    # StartPos and LoopCount are not writable from Python - the montage computes
    # the segment's position in the track itself, and a single-segment montage
    # starts at zero anyway.
    segment = unreal.AnimSegment()
    segment.set_editor_property("anim_reference", source)
    segment.set_editor_property("anim_start_time", 0.0)
    segment.set_editor_property("anim_end_time", source.get_editor_property("sequence_length"))
    segment.set_editor_property("anim_play_rate", 1.0)

    anim_track = unreal.AnimTrack()
    anim_track.set_editor_property("anim_segments", [segment])

    slot_track = unreal.SlotAnimationTrack()
    slot_track.set_editor_property("slot_name", slot_name)
    slot_track.set_editor_property("anim_track", anim_track)

    montage.set_editor_property("slot_anim_tracks", [slot_track])

    # AlphaBlend takes no constructor arguments from Python, so build and set.
    blend_in_curve = unreal.AlphaBlend()
    blend_in_curve.set_editor_property("blend_time", blend_in)
    montage.set_editor_property("blend_in", blend_in_curve)

    blend_out_curve = unreal.AlphaBlend()
    blend_out_curve.set_editor_property("blend_time", blend_out)
    montage.set_editor_property("blend_out", blend_out_curve)

    EAL.save_loaded_asset(montage)
    log("built {} from {}".format(path, source.get_name()))
    return montage


def assign(target, mapping, montages):
    for prop, (name, _clip, _in, _out) in mapping.items():
        montage = montages.get(name)
        if montage:
            target.set_editor_property(prop, montage)


def main():
    slots = verify_slot()
    slot_name = "DefaultSlot" if "DefaultSlot" in slots else slots[0]
    log("playing montages into slot '{}'".format(slot_name))

    built = {}
    for mapping in (CHARACTER_MONTAGES, TRAVERSAL_MONTAGES):
        for _prop, (name, clip, blend_in, blend_out) in mapping.items():
            montage = build_montage(name, clip, blend_in, blend_out, slot_name)
            if montage:
                built[name] = montage

    blueprint = unreal.load_asset(BP_OPERATIVE)
    if not blueprint:
        raise RuntimeError("BP_Operative not found")

    cdo = unreal.get_default_object(blueprint.generated_class())
    assign(cdo, CHARACTER_MONTAGES, built)
    assign(cdo.get_editor_property("Traversal"), TRAVERSAL_MONTAGES, built)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    EAL.save_loaded_asset(blueprint)

    log("assigned {} montages to BP_Operative".format(len(built)))
    log("done")


main()
