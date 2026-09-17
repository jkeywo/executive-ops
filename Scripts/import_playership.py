"""
Imports the player ship model and wires it onto BP_Aircraft.

Run headless:

    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/import_playership.py"

Source art lives in Raw/ (gitignored); this produces the committed assets under
/Game/Vehicles/PlayerShip. Idempotent: re-running reimports and re-wires.
"""
import os
import unreal

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

PROJECT = unreal.Paths.project_dir()
RAW = os.path.join(PROJECT, "Raw", "PlayerShip")

DEST = "/Game/Vehicles/PlayerShip"
TEX_DEST = DEST + "/Textures"

MESH_PATH = DEST + "/SM_PlayerShip"
MAT_PATH = DEST + "/M_PlayerShip"

BP_AIRCRAFT = "/Game/Blueprints/BP_Aircraft"

# The hull should read at roughly the size the collision box already assumes:
# 800 x 600 x 240cm. Fitting to the longest axis keeps the proportions honest.
TARGET_LENGTH = 800.0


def log(msg):
    unreal.log_warning("[Ship] " + msg)


def find_source():
    """Locates the .fbx and its texture folder inside Raw/PlayerShip."""
    fbx = None
    for name in os.listdir(RAW):
        if name.lower().endswith(".fbx"):
            fbx = os.path.join(RAW, name)
            break

    if not fbx:
        raise RuntimeError("no .fbx found in " + RAW)

    textures = {}
    fbm = fbx[:-4] + ".fbm"
    if os.path.isdir(fbm):
        for name in os.listdir(fbm):
            lower = name.lower()
            path = os.path.join(fbm, name)
            # Tripo names the albedo "rgb"; the rest are named plainly.
            if "rgb" in lower or "basecolor" in lower or "albedo" in lower:
                textures["BaseColor"] = path
            elif "normal" in lower:
                textures["Normal"] = path
            elif "rough" in lower:
                textures["Roughness"] = path
            elif "metal" in lower:
                textures["Metallic"] = path

    return fbx, textures


def import_mesh(fbx):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)

    static_mesh_data = options.static_mesh_import_data
    static_mesh_data.set_editor_property("combine_meshes", True)
    static_mesh_data.set_editor_property("generate_lightmap_u_vs", True)
    static_mesh_data.set_editor_property("auto_generate_collision", False)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", "SM_PlayerShip")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)

    AT.import_asset_tasks([task])

    mesh = unreal.load_asset(MESH_PATH)
    if not mesh:
        raise RuntimeError("mesh import produced nothing at " + MESH_PATH)

    log("imported mesh " + MESH_PATH)
    return mesh


def import_texture(path, slot):
    name = "T_PlayerShip_" + slot
    asset_path = TEX_DEST + "/" + name

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", TEX_DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    AT.import_asset_tasks([task])

    texture = unreal.load_asset(asset_path)
    if not texture:
        raise RuntimeError("texture import failed: " + path)

    # Only the albedo is colour data. Treating the others as sRGB washes out
    # roughness and metallic and breaks normal reconstruction.
    if slot == "Normal":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings",
                                    unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif slot in ("Roughness", "Metallic"):
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings",
                                    unreal.TextureCompressionSettings.TC_MASKS)

    EAL.save_loaded_asset(texture)
    log("imported texture " + asset_path)
    return texture


def build_material(textures):
    if EAL.does_asset_exist(MAT_PATH):
        EAL.delete_asset(MAT_PATH)

    material = AT.create_asset("M_PlayerShip", DEST, unreal.Material,
                               unreal.MaterialFactoryNew())

    slots = [
        ("BaseColor", unreal.MaterialProperty.MP_BASE_COLOR, -400, -300),
        ("Normal", unreal.MaterialProperty.MP_NORMAL, -400, 100),
        ("Roughness", unreal.MaterialProperty.MP_ROUGHNESS, -400, 300),
        ("Metallic", unreal.MaterialProperty.MP_METALLIC, -400, 500),
    ]

    for slot, prop, x, y in slots:
        texture = textures.get(slot)
        if not texture:
            log("no " + slot + " map; leaving that input at its default")
            continue

        sample = MEL.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, x, y)
        sample.texture = texture

        # Greyscale maps carry their value in R, not RGB.
        out = "RGB" if slot in ("BaseColor", "Normal") else "R"
        MEL.connect_material_property(sample, out, prop)

    MEL.recompile_material(material)
    EAL.save_loaded_asset(material)
    log("built material " + MAT_PATH)
    return material


def fit_to_hull(mesh):
    """Returns the uniform scale that makes the mesh the intended hull size."""
    bounds = mesh.get_bounds()
    extent = bounds.box_extent
    size = unreal.Vector(extent.x * 2.0, extent.y * 2.0, extent.z * 2.0)

    longest = max(size.x, size.y, size.z)
    if longest <= 0.0:
        raise RuntimeError("mesh has no size")

    scale = TARGET_LENGTH / longest
    log("mesh size {:.1f} x {:.1f} x {:.1f}cm -> uniform scale {:.4f}".format(
        size.x, size.y, size.z, scale))

    # Which axis is longest decides whether the model needs turning to face
    # down +X, which is forward for every pawn in this project.
    axis = "X" if longest == size.x else ("Y" if longest == size.y else "Z")
    log("longest axis is " + axis)
    return scale, size, axis


def wire_to_aircraft(mesh, material, scale, size, axis):
    blueprint = unreal.load_asset(BP_AIRCRAFT)
    if not blueprint:
        raise RuntimeError("BP_Aircraft not found; run m0_setup.py first")

    cdo = unreal.get_default_object(blueprint.generated_class())
    hull = cdo.get_editor_property("HullMesh")

    hull.set_editor_property("static_mesh", mesh)
    hull.set_editor_property("relative_scale3d", unreal.Vector(scale, scale, scale))

    # Turn the model so its long axis points down +X. Tripo exports are not
    # authored to any particular forward.
    yaw = -90.0 if axis == "Y" else 0.0
    hull.set_editor_property("relative_rotation",
                             unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))

    if material:
        hull.set_material(0, material)

    # The greybox thrusters were cubes standing in for exhaust. Inside a real
    # hull they just poke through it. The components stay - M8 hangs actual VFX
    # on them - but they render nothing now.
    for name in ("ThrusterLeft", "ThrusterRight"):
        thruster = cdo.get_editor_property(name)
        thruster.set_editor_property("static_mesh", None)

    # Match the collision to the hull it is now wrapping. The placeholder box was
    # sized for a cube and is far too small for this model, which would let the
    # ship visibly pass through buildings it should be hitting.
    #
    # Deliberately a little tighter than the visual bounds: the twin tails are
    # thin and vertical, and boxing them in would have the player bumping
    # invisible walls well clear of anything they can see.
    scaled = [size.x * scale, size.y * scale, size.z * scale]
    if axis == "Y":
        scaled[0], scaled[1] = scaled[1], scaled[0]

    collision = cdo.get_editor_property("CollisionBox")
    collision.set_editor_property("box_extent", unreal.Vector(
        scaled[0] * 0.48, scaled[1] * 0.46, scaled[2] * 0.32))
    log("collision extents set to {:.0f} x {:.0f} x {:.0f}".format(
        scaled[0] * 0.48, scaled[1] * 0.46, scaled[2] * 0.32))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    EAL.save_loaded_asset(blueprint)
    log("wired onto BP_Aircraft (yaw {:.0f})".format(yaw))


def main():
    log("importing player ship")

    fbx, textures = find_source()
    log("source: " + os.path.basename(fbx))

    mesh = import_mesh(fbx)

    imported = {}
    for slot, path in textures.items():
        imported[slot] = import_texture(path, slot)

    material = build_material(imported)

    if material:
        mesh.set_editor_property("static_materials",
                                 [unreal.StaticMaterial(material_interface=material)])
        EAL.save_loaded_asset(mesh)

    scale, size, axis = fit_to_hull(mesh)
    wire_to_aircraft(mesh, material, scale, size, axis)

    log("done")


main()
