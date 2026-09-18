"""
Places the functional test actors in the maps they run in.

A functional test is an actor: it lives in a level, and the automation framework
finds it there and runs it by name. Placing one is the same operation as placing
the navmesh volume, so this follows Scripts/add_navmesh.py - idempotent by
label, a re-run updates rather than duplicates.

Add a row to TESTS for each new test class. The label is what the test is
called in the Session Frontend, under Project.Functional Tests.<Map>.

Run with the editor closed:

    UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript \
        -script="Scripts/place_functional_tests.py"
"""

import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eo_editor  # noqa: E402

# (map, label, class name as the Python unreal module exposes it)
TESTS = [
    ("/Game/Maps/L_MissionTest", "MissionLoop", "EOMissionLoopTest"),
]

# Out of the way of the arena: nothing here needs to be anywhere in particular,
# and a test actor standing on the floor is one the guard can walk into.
SPAWN_AT = unreal.Vector(0.0, 0.0, 5000.0)


def log(message):
    # Display-level Python logging is filtered under -unattended; warning always
    # shows, which is the convention the rest of the scripts in here follow.
    unreal.log_warning("[tests] {}".format(message))


def existing(actors, label):
    for actor in actors:
        if actor.get_actor_label() == label:
            return actor
    return None


def place(map_path, label, class_name):
    cls = getattr(unreal, class_name, None)
    if cls is None:
        unreal.log_error("[tests] no class {} - has the project been built?".format(class_name))
        return False

    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsystem.load_level(map_path)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    actor = existing(actors, label)
    if actor is None:
        actor = actor_subsystem.spawn_actor_from_class(cls, SPAWN_AT)
        if actor is None:
            unreal.log_error("[tests] could not spawn {}".format(class_name))
            return False
        actor.set_actor_label(label)
        log("placed {} in {}".format(label, map_path))
    else:
        log("{} already in {}".format(label, map_path))

    # The framework's own name for the test, shown in the Session Frontend.
    actor.set_editor_property("test_label", label)

    eo_editor.save_current_level(subsystem, actor_subsystem)
    return True


def main():
    ok = all(place(*row) for row in TESTS)
    log("done" if ok else "finished with errors")


main()
