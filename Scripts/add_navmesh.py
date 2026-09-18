"""
Adds a navmesh bounds volume to the ground map, so the guard can path rather
than steer straight at things.

Deliberately targeted rather than folded into m0_setup.py's regeneration: that
script rebuilds both maps from scratch and rewrites Blueprint defaults, which
would take the animation graph wiring with it. This only ever adds one actor to
one map, and does nothing if it is already there.

Run with the editor closed:

    UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript \
        -script="Scripts/add_navmesh.py"
"""

import unreal

MAP = "/Game/Maps/L_MissionTest"
VOLUME_LABEL = "NavMeshBounds"

# The arena floor is a 120-unit-scaled cube at the origin, so it spans about
# 6000 either side. The volume covers it with room to spare, and enough height
# for the rooftop section of the traversal route.
EXTENT = unreal.Vector(13000.0, 13000.0, 4000.0)


def log(message):
    # Display-level Python logging is filtered under -unattended; warning always
    # shows, which is the convention the rest of the scripts in here follow.
    unreal.log_warning("[navmesh] {}".format(message))


def existing(actors, label):
    for actor in actors:
        if actor.get_actor_label() == label:
            return actor
    return None


def main():
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsystem.load_level(MAP)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    volume = existing(actors, VOLUME_LABEL)
    if volume is None:
        volume = actor_subsystem.spawn_actor_from_class(
            unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 0.0)
        )
        if volume is None:
            unreal.log_error("[navmesh] could not spawn the bounds volume")
            return
        volume.set_actor_label(VOLUME_LABEL)
        log("added {}".format(VOLUME_LABEL))
    else:
        log("{} already present; resizing".format(VOLUME_LABEL))

    # A brush's size is its builder brush extent times the actor scale. Scaling
    # the actor is the part that survives a reload, so that is what is set.
    volume.set_actor_scale3d(unreal.Vector(
        EXTENT.x / 100.0, EXTENT.y / 100.0, EXTENT.z / 100.0))
    volume.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, False)

    subsystem.save_current_level()
    log("saved {}".format(MAP))


main()
