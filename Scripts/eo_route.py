"""
The ground route, as one function two maps share.

L_MissionTest is the standalone arena the ground tests run in. L_SiteArena is
the same route streamed into the flight map on the mission building's roof
(Scripts/build_site_arena.py, ADR 0009). Keeping the block list here means the
two cannot drift: change the route once and rebuild both.

Import with the script's own directory on sys.path, as eo_editor is.
"""

import unreal


def place_route(spawn_block, cube, mat, guard_bp=None, floor_z=-50):
    """
    M4's ground movement route: every traversal verb the milestone asks for, laid
    out as a short circuit that can be run repeatedly.

    Reads left to right from the insertion point:
      open sprint -> low vault -> slide gap -> mantle ledge -> climb wall ->
      rooftop with a jump gap -> drop down -> corner -> back to the start.

    Two ways through the middle section: over the high wall, or around it via
    the low route. Neither is meant to be optimal - the point is that the player
    has a choice and neither one demands precise platforming.

    spawn_block(location, scale, label, material) places one greybox cube.
    floor_z is the floor slab's centre: -50 puts its top at the origin, which is
    where the route is authored from.
    """
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    def block(x, y, z, sx, sy, sz, label):
        spawn_block(unreal.Vector(x, y, z), unreal.Vector(sx, sy, sz), label, mat)

    # Floor.
    block(0, 0, floor_z, 120, 120, 1, "Ground")

    # --- Open sprint --------------------------------------------------------
    # Nothing here on purpose: the route needs a stretch to build up speed in
    # before the first obstacle, or sprint and slide never get exercised.

    # --- Low vault ----------------------------------------------------------
    # 90cm: under the 130cm vault threshold, so it is cleared without stopping.
    block(1800, 0, 45, 1.5, 12, 0.9, "Vault_Low")

    # --- Slide gap ----------------------------------------------------------
    # A bar at head height with clear floor under it: has to be slid beneath.
    block(3200, 0, 200, 2, 12, 0.6, "Slide_Bar_Underside")
    block(3200, -700, 130, 2, 2, 2.6, "Slide_Bar_PostL")
    block(3200, 700, 130, 2, 2, 2.6, "Slide_Bar_PostR")

    # --- Mantle ledge -------------------------------------------------------
    # 180cm: too tall to vault, low enough to pull up onto and stand.
    block(4600, 0, 90, 3, 12, 1.8, "Mantle_Ledge")

    # --- Branch: high wall, or the low route around it -----------------------
    # Route A: a 380cm face to scramble up, ending on the upper deck.
    block(6200, -300, 190, 3, 6, 3.8, "Climb_Wall")
    block(7000, -300, 370, 10, 6, 0.4, "Upper_Deck")

    # Route B: around the side at ground level, longer but no climbing.
    block(6200, 500, 60, 3, 1, 1.2, "LowRoute_Step_A")
    block(6800, 800, 60, 3, 1, 1.2, "LowRoute_Step_B")

    # --- Jump gap on the upper deck -----------------------------------------
    # The two decks are 500cm apart: a committed jump, not a step across.
    block(8500, -300, 370, 10, 6, 0.4, "Upper_Deck_Far")

    # --- Drop down and corner ------------------------------------------------
    block(9800, -300, 180, 3, 6, 3.6, "Drop_Ledge")
    block(9800, 1200, 45, 12, 1.5, 0.9, "Corner_Vault")

    # --- Elevation change back to the start ----------------------------------
    for i in range(4):
        block(9000 - i * 900, 2400, 40 + i * 60, 4, 4, 0.8 + i * 1.2,
              "Stair_{}".format(i))

    # --- The guard ----------------------------------------------------------
    # Placed past the slide gap, patrolling across the approach to the mantle
    # ledge, so the player meets it mid-route with several ways to handle it:
    # sneak up behind, shoot it, or slide past and keep going.
    if guard_bp:
        guard = actor_subsystem.spawn_actor_from_class(
            guard_bp.generated_class(), unreal.Vector(3900, -600, 100),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0))
        guard.set_actor_label("Guard")

    # --- Objective ----------------------------------------------------------
    # At the far end of the route, past the guard: reaching it is the mission.
    terminal = actor_subsystem.spawn_actor_from_class(
        unreal.EOObjectiveTerminal, unreal.Vector(9800, 1200, 190))
    terminal.set_actor_label("ObjectiveTerminal")
    tbody = terminal.get_editor_property("Body")
    tbody.set_editor_property("static_mesh", cube)
    tbody.set_editor_property("relative_scale3d", unreal.Vector(1.2, 1.2, 1.8))
    if mat:
        tbody.set_material(0, mat)
    tbeacon = terminal.get_editor_property("Beacon")
    tbeacon.set_editor_property("static_mesh", cube)
    if mat:
        tbeacon.set_material(0, mat)

    # --- Extraction ---------------------------------------------------------
    # Back near the insertion point, so leaving means crossing the arena again -
    # past a guard that may now be looking for you.
    extraction = actor_subsystem.spawn_actor_from_class(
        unreal.EOExtractionZone, unreal.Vector(600, 2600, 96))
    extraction.set_actor_label("ExtractionZone")
    for prop in ("Pad", "Beacon"):
        comp = extraction.get_editor_property(prop)
        comp.set_editor_property("static_mesh", cube)
        if mat:
            comp.set_material(0, mat)
