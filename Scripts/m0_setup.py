"""
M0 editor setup, run headless.

Creates the two maps, the greybox flight playground, and the Blueprint subclasses
that hold asset references. Idempotent: safe to re-run.

    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/m0_setup.py"
"""
import unreal

EAL = unreal.EditorAssetLibrary
ELL = unreal.EditorLevelLibrary
ELAS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
AT = unreal.AssetToolsHelpers.get_asset_tools()

CUBE = "/Engine/BasicShapes/Cube.Cube"
BASIC_MAT = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"

MAPS = "/Game/Maps"
BP = "/Game/Blueprints"

SKELETON = "/Game/OpenWorldAnimset/UE4_Mannequin/Mesh/UE4_Mannequin_Skeleton"

OWA = "/Game/OpenWorldAnimset/Animations"

# Locomotion and traversal clips from the owned Open World set. Kept here rather
# than in C++ so the code carries no asset paths.
ANIMS = {
    "IdleAnim":   OWA + "/Idles/Idle",
    "WalkAnim":   OWA + "/FMotion/Walk/FMotion_Walk_Fast_Loop",
    "JogAnim":    OWA + "/FMotion/Jog/FMotion_Jog_Fast_Loop",
    "SprintAnim": OWA + "/FMotion/Sprint/FMotion_Sprint_Loop",
    "FallAnim":   OWA + "/Jump/Jog_Jump_Start",
    "SlideAnim":  OWA + "/Slide/Slide_Idle",
}

GUARD_ANIMS = {
    "IdleAnim":      OWA + "/Idles/Idle",
    "WalkAnim":      OWA + "/FMotion/Walk/FMotion_Walk_Fast_Loop",
    "AimAnim":       OWA + "/Pistol/Pistol_aim_Idle",
    "FireAnim":      OWA + "/Pistol/Pistol_shoot_01",
    "HitReactAnim":  OWA + "/FMotion/Injured/FMotion_Injured_Idle",
    "DeathAnim":     OWA + "/Deaths/death_aim_back_01",
}

COMBAT_ANIMS = {
    "AimAnim":      OWA + "/Pistol/Pistol_aim_Idle",
    "FireAnim":     OWA + "/Pistol/Pistol_shoot_01",
    "DeathAnim":    OWA + "/Deaths/death_aim_chest_01",
    # No bespoke assassination in the packs; one contextual strike is enough.
    "TakedownAnim": OWA + "/NPC/Anim_TA_ANG_hit_fist",
}

TRAVERSAL_ANIMS = {
    "VaultAnim":  OWA + "/Vault/Vault_jog",
    "MantleAnim": OWA + "/Ledge/High_Ledge_Up_Crouch",
    "ClimbAnim":  OWA + "/Climb/Climb_scrambling_path",
}
SK_MESH = "/Game/OpenWorldAnimset/UE4_Mannequin/Mesh/SK_Mannequin"


def log(msg):
    # Display-level Python logging is filtered under -unattended; warning always shows.
    unreal.log_warning("[M0] " + msg)


# ---------------------------------------------------------------- blueprints

def make_blueprint(path, parent_class):
    """Create a Blueprint subclass of a C++ class, or return the existing one."""
    if EAL.does_asset_exist(path):
        log("exists: " + path)
        return unreal.load_asset(path)

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    pkg_path, name = path.rsplit("/", 1)
    asset = AT.create_asset(name, pkg_path, unreal.Blueprint, factory)
    EAL.save_loaded_asset(asset)
    log("created blueprint: " + path)
    return asset


def create_blueprints():
    unreal.EditorAssetLibrary.make_directory(BP)

    aircraft = make_blueprint(BP + "/BP_Aircraft", unreal.EOAircraftPawn)
    guard = make_blueprint(BP + "/BP_Guard", unreal.EOGuardCharacter)
    operative = make_blueprint(BP + "/BP_Operative", unreal.EOOperativeCharacter)
    controller = make_blueprint(BP + "/BP_PlayerController", unreal.EOPlayerController)
    gamemode = make_blueprint(BP + "/BP_GameMode", unreal.EOGameModeBase)

    # Aircraft hull: a stretched cube so the greybox VTOL is visible and readable.
    cube = unreal.load_asset(CUBE)
    mat = unreal.load_asset(BASIC_MAT)

    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

    def set_defaults(bp_asset, setter):
        """Edit a Blueprint's CDO, then recompile and save."""
        cdo = unreal.get_default_object(bp_asset.generated_class())
        setter(cdo)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp_asset)
        EAL.save_loaded_asset(bp_asset)

    def cfg_aircraft(cdo):
        hull = cdo.get_editor_property("HullMesh")
        hull.set_editor_property("static_mesh", cube)
        hull.set_editor_property("relative_scale3d", unreal.Vector(8.0, 6.0, 2.0))
        if mat:
            hull.set_material(0, mat)

        # Thrusters are plain cubes that swell with engine load - a readable
        # greybox stand-in for exhaust VFX, needing no particle asset.
        for name in ("ThrusterLeft", "ThrusterRight"):
            thruster = cdo.get_editor_property(name)
            thruster.set_editor_property("static_mesh", cube)
            if mat:
                thruster.set_material(0, mat)

    def assign_anims(target, mapping):
        for prop, path in mapping.items():
            anim = unreal.load_asset(path)
            if not anim:
                raise RuntimeError("animation not found: " + path)
            target.set_editor_property(prop, anim)

    def cfg_operative(cdo):
        # ACharacter's skeletal mesh component is the UPROPERTY named "Mesh".
        mesh = cdo.get_editor_property("Mesh")
        sk = unreal.load_asset(SK_MESH)
        if not sk:
            raise RuntimeError("skeletal mesh not found: " + SK_MESH)
        try:
            mesh.set_skeletal_mesh_asset(sk)
        except AttributeError:
            mesh.set_editor_property("skeletal_mesh_asset", sk)
        # Stand the mannequin up inside the capsule.
        mesh.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -96.0))
        mesh.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, -90.0))

        # Single-node playback: the character drives clips directly from C++.
        mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)

        assign_anims(cdo, ANIMS)
        assign_anims(cdo, COMBAT_ANIMS)
        assign_anims(cdo.get_editor_property("Traversal"), TRAVERSAL_ANIMS)

    def cfg_controller(cdo):
        cdo.set_editor_property("AircraftClass", aircraft.generated_class())
        cdo.set_editor_property("OperativeClass", operative.generated_class())

    def cfg_gamemode(cdo):
        cdo.set_editor_property("player_controller_class", controller.generated_class())
        cdo.set_editor_property("default_pawn_class", operative.generated_class())

    def cfg_guard(cdo):
        mesh = cdo.get_editor_property("Mesh")
        sk = unreal.load_asset(SK_MESH)
        if sk:
            mesh.set_skeletal_mesh_asset(sk)
        mesh.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -96.0))
        mesh.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, -90.0))
        mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)

        assign_anims(cdo, GUARD_ANIMS)

        # A there-and-back patrol across the objective approach, so the player
        # meets the guard whichever way they come in.
        cdo.set_editor_property("PatrolOffsets", [
            unreal.Vector(0.0, 0.0, 0.0),
            unreal.Vector(1400.0, 0.0, 0.0),
            unreal.Vector(1400.0, 1200.0, 0.0),
            unreal.Vector(0.0, 1200.0, 0.0),
        ])

    set_defaults(guard, cfg_guard)
    set_defaults(aircraft, cfg_aircraft)
    set_defaults(operative, cfg_operative)
    set_defaults(controller, cfg_controller)
    set_defaults(gamemode, cfg_gamemode)

    log("blueprints configured")
    return aircraft, operative, controller, gamemode, guard


# ---------------------------------------------------------------- level build

def spawn_block(location, scale, label, material):
    """
    One greybox box. Scale is in cube units (a cube is 100cm).

    Spawns a StaticMeshActor by class and assigns the mesh, rather than using
    spawn_actor_from_object, which returns None under the commandlet.
    """
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


def add_lighting():
    """Minimum needed for the map to be visible."""
    sun = EAS.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-46, -30, 0))
    sun.set_actor_label("Sun")
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp:
        sun_comp.set_editor_property("intensity", 4.0)

    sky_light = EAS.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2000))
    sky_light.set_actor_label("SkyLight")
    sky_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
    if sky_comp:
        sky_comp.set_editor_property("real_time_capture", True)

    EAS.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label("SkyAtmosphere")
    EAS.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0)).set_actor_label("HeightFog")


def fresh_level(path):
    """
    Start from an empty level at path.

    A previously generated level cannot be deleted while loaded, and new_level
    refuses to overwrite, so an existing one is loaded and emptied instead.
    """
    if EAL.does_asset_exist(path):
        if not ELAS.load_level(path):
            raise RuntimeError("could not load existing level " + path)
        actors = EAS.get_all_level_actors()
        for actor in actors:
            EAS.destroy_actor(actor)
        log("cleared {} actor(s) from existing {}".format(len(actors), path))
    elif not ELAS.new_level(path):
        raise RuntimeError("could not create level " + path)


def save_level(path):
    if not ELAS.save_current_level():
        raise RuntimeError("could not save level " + path)


def build_flight_map():
    """
    M2's city district: a small block of towers with street canyons between them,
    rooftop levels, elevated infrastructure to fly under, and one obvious mission
    building. Minutes of environment, not kilometres.

    Deliberately crude geometry. Its job is to answer questions about aircraft
    scale, useful speed, building spacing and camera readability - not to look
    like a city.
    """
    fresh_level(MAPS + "/L_FlightTest")
    add_lighting()

    mat = unreal.load_asset(BASIC_MAT)

    # Ground. 1km square: enough that the district has edges the player can see.
    spawn_block(unreal.Vector(0, 0, -50), unreal.Vector(500, 500, 1), "Ground", mat)

    # --- Tower blocks ------------------------------------------------------
    # A 4x4 grid on a 5000cm pitch. Gaps between towers form the street canyons;
    # varied heights give rooftop levels at different altitudes to work from.
    pitch = 5000
    heights = [34, 58, 22, 71, 45, 88, 27, 63, 40, 52, 76, 31, 68, 25, 55, 82]
    mission_cell = (1, 1)

    index = 0
    for gx in range(-2, 2):
        for gy in range(-2, 2):
            index += 1
            if (gx, gy) == mission_cell:
                continue

            h = heights[index % len(heights)]
            x, y = gx * pitch + pitch // 2, gy * pitch + pitch // 2
            spawn_block(unreal.Vector(x, y, h * 50), unreal.Vector(16, 16, h),
                        "Tower_{}_{}".format(gx, gy), mat)

            # Rooftop structures: something to hover beside and read height against.
            if h > 50:
                spawn_block(unreal.Vector(x + 400, y + 400, h * 100 + 300),
                            unreal.Vector(4, 4, 6), "Roof_{}_{}".format(gx, gy), mat)

    # --- The mission building ----------------------------------------------
    # Lower and wider than its neighbours, with a clear flat roof, so it reads as
    # the objective from the air without needing a label.
    mx, my = mission_cell[0] * pitch + pitch // 2, mission_cell[1] * pitch + pitch // 2
    spawn_block(unreal.Vector(mx, my, 1400), unreal.Vector(26, 26, 28),
                "MissionBuilding", mat)

    # --- Elevated infrastructure -------------------------------------------
    # A skyway across the district at mid altitude: an obstacle at speed and an
    # obvious "fly under or over" decision.
    spawn_block(unreal.Vector(-2500, 0, 4200), unreal.Vector(6, 200, 1), "Skyway", mat)
    for i in range(-4, 5):
        spawn_block(unreal.Vector(-2500, i * 2500, 2100),
                    unreal.Vector(2, 2, 42), "SkywayPillar_{}".format(i), mat)

    # --- Tighter optional route --------------------------------------------
    # A narrow slot between two slabs, off the obvious approach line. Wide enough
    # for the craft, tight enough to be a choice rather than a default.
    spawn_block(unreal.Vector(7000, -3000, 3000), unreal.Vector(30, 4, 60), "SlotWall_A", mat)
    spawn_block(unreal.Vector(7000, -1400, 3000), unreal.Vector(30, 4, 60), "SlotWall_B", mat)

    # --- Mission site -------------------------------------------------------
    # Hover volume sits above the mission building's roof; the operative inserts
    # onto the roof itself.
    site = EAS.spawn_actor_from_class(
        unreal.EOMissionSite, unreal.Vector(mx, my, 4200))
    site.set_actor_label("MissionSite")
    marker = site.get_editor_property("Marker")
    marker.set_editor_property("static_mesh", unreal.load_asset(CUBE))
    if mat:
        marker.set_material(0, mat)

    start = EAS.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(-11000, -11000, 2500), unreal.Rotator(0, 45, 0))
    start.set_actor_label("PlayerStart")

    save_level(MAPS + "/L_FlightTest")
    log("built L_FlightTest: {} actors".format(len(EAS.get_all_level_actors())))


def build_mission_map(guard_bp=None):
    """
    M4's ground movement route: every traversal verb the milestone asks for, laid
    out as a short circuit that can be run repeatedly.

    Reads left to right from the insertion point:
      open sprint -> low vault -> slide gap -> mantle ledge -> climb wall ->
      rooftop with a jump gap -> drop down -> corner -> back to the start.

    Two ways through the middle section: over the high wall, or around it via
    the low route. Neither is meant to be optimal - the point is that the player
    has a choice and neither one demands precise platforming.
    """
    fresh_level(MAPS + "/L_MissionTest")
    add_lighting()

    mat = unreal.load_asset(BASIC_MAT)

    def block(x, y, z, sx, sy, sz, label):
        spawn_block(unreal.Vector(x, y, z), unreal.Vector(sx, sy, sz), label, mat)

    # Floor.
    block(0, 0, -50, 120, 120, 1, "Ground")

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
        guard = EAS.spawn_actor_from_class(
            guard_bp.generated_class(), unreal.Vector(3900, -600, 100),
            unreal.Rotator(0, 90, 0))
        guard.set_actor_label("Guard")

    start = EAS.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, 0, 200), unreal.Rotator(0, 0, 0))
    start.set_actor_label("PlayerStart")

    save_level(MAPS + "/L_MissionTest")
    log("built L_MissionTest: {} actors".format(len(EAS.get_all_level_actors())))


def place_pawns_in_flight_map(aircraft_bp):
    """
    Put a real aircraft in the flight map so the controller's spawn fallback is
    not doing the work.
    """
    if not ELAS.load_level(MAPS + "/L_FlightTest"):
        raise RuntimeError("could not load L_FlightTest")

    existing = [a for a in EAS.get_all_level_actors()
                if a.get_class() == aircraft_bp.generated_class()]
    if existing:
        log("aircraft already placed")
    else:
        craft = EAS.spawn_actor_from_class(
            aircraft_bp.generated_class(),
            unreal.Vector(-11000, -11000, 2500), unreal.Rotator(0, 45, 0))
        craft.set_actor_label("BP_Aircraft")
        log("placed aircraft in L_FlightTest")

    save_level(MAPS + "/L_FlightTest")


def report_gamemode_path(gamemode_bp):
    """
    Report the game mode's class path for Config/DefaultEngine.ini.

    The config is written outside this script: unreal.ConfigCacheIni is not exposed
    to Python, and writing the .ini here would race the editor's own config flush
    on shutdown.
    """
    log("GAMEMODE_PATH=" + gamemode_bp.generated_class().get_path_name())


def verify():
    """Re-load everything from disk and confirm the defaults actually persisted."""
    problems = []

    def check(cond, msg):
        if cond:
            log("  OK   " + msg)
        else:
            log("  FAIL " + msg)
            problems.append(msg)

    log("verifying:")

    for path in [MAPS + "/L_FlightTest", MAPS + "/L_MissionTest",
                 BP + "/BP_Aircraft", BP + "/BP_Operative",
                 BP + "/BP_PlayerController", BP + "/BP_GameMode", BP + "/BP_Guard"]:
        check(EAL.does_asset_exist(path), "asset exists: " + path)

    if ELAS.load_level(MAPS + "/L_MissionTest"):
        guards = [a for a in EAS.get_all_level_actors()
                  if isinstance(a, unreal.EOGuardCharacter)]
        check(len(guards) == 1, "L_MissionTest has exactly 1 guard (found {})".format(len(guards)))

    # Existence is not enough - load each map and confirm it actually has content.
    for path, min_actors in [(MAPS + "/L_MissionTest", 20), (MAPS + "/L_FlightTest", 40)]:
        if not ELAS.load_level(path):
            check(False, "could not load " + path)
            continue
        actors = EAS.get_all_level_actors()
        labels = [a.get_actor_label() for a in actors]
        check(len(actors) >= min_actors,
              "{} has {} actors (expected >= {})".format(path, len(actors), min_actors))
        check("PlayerStart" in labels, path + " has a PlayerStart")
        check(any(isinstance(a, unreal.DirectionalLight) for a in actors),
              path + " has a directional light")

    # The flight map must contain a real aircraft, not rely on the spawn fallback.
    if ELAS.load_level(MAPS + "/L_FlightTest"):
        actors = EAS.get_all_level_actors()

        craft = [a for a in actors if isinstance(a, unreal.EOAircraftPawn)]
        check(len(craft) == 1, "L_FlightTest has exactly 1 aircraft (found {})".format(len(craft)))

        sites = [a for a in actors if isinstance(a, unreal.EOMissionSite)]
        check(len(sites) == 1, "L_FlightTest has exactly 1 mission site (found {})".format(len(sites)))

        if craft and sites:
            # The approach has to be a flight, not a hop.
            dist = craft[0].get_actor_location().distance(sites[0].get_actor_location())
            check(dist > 15000,
                  "start is a real distance from the site ({:.0f}cm)".format(dist))

    op = unreal.load_asset(BP + "/BP_Operative")
    if op:
        cdo = unreal.get_default_object(op.generated_class())
        mesh = cdo.get_editor_property("Mesh")
        assigned = mesh.get_editor_property("skeletal_mesh_asset")
        check(assigned is not None, "operative skeletal mesh assigned: {}".format(assigned))

    pc = unreal.load_asset(BP + "/BP_PlayerController")
    if pc:
        cdo = unreal.get_default_object(pc.generated_class())
        check(cdo.get_editor_property("AircraftClass") is not None, "controller AircraftClass set")
        check(cdo.get_editor_property("OperativeClass") is not None, "controller OperativeClass set")

    gm = unreal.load_asset(BP + "/BP_GameMode")
    if gm:
        cdo = unreal.get_default_object(gm.generated_class())
        check(cdo.get_editor_property("player_controller_class") is not None, "gamemode controller set")
        check(cdo.get_editor_property("default_pawn_class") is not None, "gamemode pawn set")

    if problems:
        log("VERIFY FAILED: {} problem(s)".format(len(problems)))
    else:
        log("VERIFY PASSED")
    return not problems


def main():
    log("starting M0 setup")
    unreal.EditorAssetLibrary.make_directory(MAPS)

    aircraft, operative, controller, gamemode, guard = create_blueprints()

    build_mission_map(guard)
    build_flight_map()
    place_pawns_in_flight_map(aircraft)

    report_gamemode_path(gamemode)

    unreal.EditorAssetLibrary.save_directory("/Game/Maps", only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory("/Game/Blueprints", only_if_is_dirty=False)
    verify()
    log("M0 setup complete")


main()
