"""
Shared steps for scripts that save a map from the headless editor.

Import with the script's own directory on sys.path:

    import os, sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import eo_editor
"""

import unreal


def log(message):
    # Display-level Python logging is filtered under -unattended; warning always
    # shows, which is the convention the rest of the scripts in here follow.
    unreal.log_warning("[editor] {}".format(message))


def strip_saved_navmesh(actor_subsystem):
    """
    Removes any RecastNavMesh from the current level before it is saved.

    The navmesh is generated at runtime, and the engine only rebuilds at
    startup the navigation data it spawned itself to fill a gap: one loaded from
    the map is trusted as saved. A commandlet has no tick loop, so nothing
    builds tiles between the editor auto-spawning a RecastNavMesh on load and
    the script saving the level - and a saved, empty navmesh is one the guard
    can never path on. Leaving the map with only its bounds volume means the
    runtime spawns a fresh one and builds it, every launch, which is the
    behaviour Config/DefaultEngine.ini describes.
    """
    removed = 0
    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.RecastNavMesh):
            actor_subsystem.destroy_actor(actor)
            removed += 1
    if removed:
        log("stripped {} saved navmesh actor(s); the runtime builds its own".format(removed))
    return removed


def save_current_level(level_subsystem, actor_subsystem):
    """The one way a script here saves a level: strip, then save."""
    strip_saved_navmesh(actor_subsystem)
    return level_subsystem.save_current_level()
