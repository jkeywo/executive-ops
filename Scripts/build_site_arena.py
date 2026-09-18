"""
Builds the arena that streams into the flight map, and points the site at it.

L_SiteArena is the ground route from eo_route with nothing that would double
up against the flight map: no lighting, no PlayerStart. Its origin is the
route's own start, and AEOMissionSite streams it in with that origin on the
insertion point - the mission building's roof (ADR 0009).

Deliberately separate from m0_setup.py, which rebuilds both test maps from
scratch and rewrites Blueprint defaults. This only ever writes one map and one
property on one actor, and is safe to re-run.

Run with the editor closed:

    UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript \
        -script="Scripts/build_site_arena.py"
"""

import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eo_editor  # noqa: E402
import eo_route  # noqa: E402

ARENA = "/Game/Maps/L_SiteArena"
FLIGHT_MAP = "/Game/Maps/L_FlightTest"
GUARD_BP = "/Game/Blueprints/BP_Guard"

CUBE = "/Engine/BasicShapes/Cube.Cube"
BASIC_MAT = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"

# The floor slab's top would otherwise sit exactly on the roof it is placed on:
# two coplanar faces flicker and give the operative two surfaces to land on.
# Two centimetres up and the deck is unambiguously the ground.
FLOOR_Z = -48

# Covers the route with room to spare, and only a few metres of height: on the
# roof a tall volume would reach the street and build tiles nobody walks on.
NAV_LABEL = "NavMeshBounds"
NAV_EXTENT = unreal.Vector(13000.0, 13000.0, 800.0)
NAV_CENTRE = unreal.Vector(0.0, 0.0, 300.0)

EAL = unreal.EditorAssetLibrary
ELAS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def log(message):
    # Display-level Python logging is filtered under -unattended; warning always
    # shows, which is the convention the rest of the scripts in here follow.
    unreal.log_warning("[arena] {}".format(message))


def spawn_block(location, scale, label, material):
    """One greybox box; scale is in cube units. Mirrors m0_setup.spawn_block."""
    cube = unreal.load_asset(CUBE)
    if not cube:
        raise RuntimeError("could not load " + CUBE)

    actor = EAS.spawn_actor_from_class(unreal.StaticMeshActor, location)
    if not actor:
        raise RuntimeError("spawn failed for " + label)

    comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    # Mobility must be movable to set the mesh, then locked down afterwards.
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(cube)
    if material:
        comp.set_material(0, material)

    actor.set_actor_scale3d(scale)
    actor.set_actor_label(label)
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    return actor


def fresh_level(path):
    """An empty level at path: a new one, or the existing one cleared out."""
    if EAL.does_asset_exist(path):
        if not ELAS.load_level(path):
            raise RuntimeError("could not load existing level " + path)
        actors = EAS.get_all_level_actors()
        for actor in actors:
            EAS.destroy_actor(actor)
        log("cleared {} actor(s) from existing {}".format(len(actors), path))
    elif not ELAS.new_level(path):
        raise RuntimeError("could not create level " + path)


def build_arena():
    fresh_level(ARENA)

    guard_bp = unreal.load_asset(GUARD_BP)
    if not guard_bp:
        raise RuntimeError("guard blueprint missing; run m0_setup.py first")

    eo_route.place_route(spawn_block, unreal.load_asset(CUBE),
                         unreal.load_asset(BASIC_MAT), guard_bp, floor_z=FLOOR_Z)

    volume = EAS.spawn_actor_from_class(unreal.NavMeshBoundsVolume, NAV_CENTRE)
    if not volume:
        raise RuntimeError("could not spawn the nav bounds volume")
    volume.set_actor_label(NAV_LABEL)
    # A brush's size is its builder brush extent times the actor scale. Scaling
    # the actor is the part that survives a reload, so that is what is set.
    volume.set_actor_scale3d(unreal.Vector(
        NAV_EXTENT.x / 100.0, NAV_EXTENT.y / 100.0, NAV_EXTENT.z / 100.0))

    if not eo_editor.save_current_level(ELAS, EAS):
        raise RuntimeError("could not save " + ARENA)
    log("built {}: {} actors".format(ARENA, len(EAS.get_all_level_actors())))


def assign_arena_to_site():
    if not ELAS.load_level(FLIGHT_MAP):
        raise RuntimeError("could not load " + FLIGHT_MAP)

    sites = [a for a in EAS.get_all_level_actors() if isinstance(a, unreal.EOMissionSite)]
    if len(sites) != 1:
        raise RuntimeError("{} has {} mission sites; expected 1".format(FLIGHT_MAP, len(sites)))

    # load_asset on a map package hands back its UWorld without opening it, and a
    # soft object property takes the object and stores the path.
    arena = unreal.load_asset(ARENA)
    if not arena:
        raise RuntimeError("could not load " + ARENA)

    sites[0].set_editor_property("ArenaLevel", arena)

    if not eo_editor.save_current_level(ELAS, EAS):
        raise RuntimeError("could not save " + FLIGHT_MAP)
    log("{} now streams {}".format(sites[0].get_actor_label(), ARENA))


def verify():
    ok = True

    if ELAS.load_level(ARENA):
        actors = EAS.get_all_level_actors()
        for label, cls in [("guard", unreal.EOGuardCharacter),
                           ("objective", unreal.EOObjectiveTerminal),
                           ("extraction zone", unreal.EOExtractionZone),
                           ("nav bounds", unreal.NavMeshBoundsVolume)]:
            found = [a for a in actors if isinstance(a, cls)]
            if len(found) != 1:
                log("FAIL {} has {} {}".format(ARENA, len(found), label))
                ok = False
        # No RecastNavMesh check: the editor spawns one the moment a level with
        # nav bounds is loaded, so a loaded level can never answer whether one
        # was saved. eo_editor strips it before every save; that is the guard.
        for cls in (unreal.PlayerStart, unreal.DirectionalLight, unreal.SkyLight,
                    unreal.SkyAtmosphere, unreal.ExponentialHeightFog):
            if any(isinstance(a, cls) for a in actors):
                log("FAIL {} contains a {}".format(ARENA, cls.__name__))
                ok = False
    else:
        log("FAIL could not load " + ARENA)
        ok = False

    if ELAS.load_level(FLIGHT_MAP):
        sites = [a for a in EAS.get_all_level_actors() if isinstance(a, unreal.EOMissionSite)]
        assigned = sites[0].get_editor_property("ArenaLevel") if sites else None
        if not assigned:
            log("FAIL the site has no ArenaLevel")
            ok = False
    else:
        log("FAIL could not load " + FLIGHT_MAP)
        ok = False

    log("VERIFY PASSED" if ok else "VERIFY FAILED")
    return ok


def main():
    # Each map is saved once, through eo_editor, at the point it is built. A
    # blanket save of the directory afterwards would reload the arena, pick up
    # the navmesh the editor spawns on load, and write it back in.
    build_arena()
    assign_arena_to_site()
    verify()


main()
