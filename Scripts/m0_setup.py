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

    def cfg_controller(cdo):
        cdo.set_editor_property("AircraftClass", aircraft.generated_class())
        cdo.set_editor_property("OperativeClass", operative.generated_class())

    def cfg_gamemode(cdo):
        cdo.set_editor_property("player_controller_class", controller.generated_class())
        cdo.set_editor_property("default_pawn_class", operative.generated_class())

    set_defaults(aircraft, cfg_aircraft)
    set_defaults(operative, cfg_operative)
    set_defaults(controller, cfg_controller)
    set_defaults(gamemode, cfg_gamemode)

    log("blueprints configured")
    return aircraft, operative, controller, gamemode


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


def build_flight_map():
    """
    M1's flight playground, roughed in: towers, canyons, pillars, a gap to fly
    through, and one obvious mission building. Deliberately crude - M1 owns the
    real layout, this exists so M0 has somewhere to fly.
    """
    fresh_level(MAPS + "/L_FlightTest")
    add_lighting()

    mat = unreal.load_asset(BASIC_MAT)

    # Ground plane.
    spawn_block(unreal.Vector(0, 0, -50), unreal.Vector(400, 400, 1), "Ground", mat)

    # A grid of towers of varying height, forming street canyons between them.
    spacing = 4000
    heights = [30, 55, 20, 70, 40, 85, 25, 60, 45]
    i = 0
    for gx in range(-1, 2):
        for gy in range(-1, 2):
            if gx == 0 and gy == 0:
                i += 1
                continue  # centre is reserved for the mission building
            h = heights[i % len(heights)]
            spawn_block(
                unreal.Vector(gx * spacing, gy * spacing, h * 50),
                unreal.Vector(12, 12, h),
                "Tower_{}_{}".format(gx, gy), mat)
            i += 1

    # The obvious mission building: wider, lower, with a flat roof to hover over.
    spawn_block(unreal.Vector(0, 0, 900), unreal.Vector(20, 20, 18), "MissionBuilding", mat)

    # Elevated infrastructure to fly under, and pillars holding it up.
    spawn_block(unreal.Vector(0, 9000, 1800), unreal.Vector(90, 6, 1), "Skyway", mat)
    for x in range(-3, 4):
        spawn_block(unreal.Vector(x * 2500, 9000, 875),
                    unreal.Vector(2, 2, 17), "SkywayPillar_{}".format(x), mat)

    # A tight gap: two blocks with a deliberately narrow slot between them.
    spawn_block(unreal.Vector(-9000, 0, 1500), unreal.Vector(6, 20, 30), "GapWall_L", mat)
    spawn_block(unreal.Vector(-9000, 3000, 1500), unreal.Vector(6, 20, 30), "GapWall_R", mat)

    start = EAS.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0, -9000, 1200), unreal.Rotator(0, 90, 0))
    start.set_actor_label("PlayerStart")

    save_level(MAPS + "/L_FlightTest")
    log("built L_FlightTest: {} actors".format(len(EAS.get_all_level_actors())))


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


def build_mission_map():
    """
    M0 only needs this map to exist and boot. A floor, lighting and a player
    start. M4/M6 build the actual route and arena.
    """
    fresh_level(MAPS + "/L_MissionTest")
    add_lighting()

    mat = unreal.load_asset(BASIC_MAT)
    spawn_block(unreal.Vector(0, 0, -50), unreal.Vector(60, 60, 1), "Ground", mat)

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
            unreal.Vector(0, -9000, 1200), unreal.Rotator(0, 90, 0))
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
                 BP + "/BP_PlayerController", BP + "/BP_GameMode"]:
        check(EAL.does_asset_exist(path), "asset exists: " + path)

    # Existence is not enough - load each map and confirm it actually has content.
    for path, min_actors in [(MAPS + "/L_MissionTest", 6), (MAPS + "/L_FlightTest", 20)]:
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
        craft = [a for a in EAS.get_all_level_actors()
                 if isinstance(a, unreal.EOAircraftPawn)]
        check(len(craft) == 1, "L_FlightTest has exactly 1 aircraft (found {})".format(len(craft)))

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

    aircraft, operative, controller, gamemode = create_blueprints()

    build_mission_map()
    build_flight_map()
    place_pawns_in_flight_map(aircraft)

    report_gamemode_path(gamemode)

    unreal.EditorAssetLibrary.save_directory("/Game/Maps", only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory("/Game/Blueprints", only_if_is_dirty=False)
    verify()
    log("M0 setup complete")


main()
