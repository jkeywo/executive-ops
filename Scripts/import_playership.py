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


MI_PATH = DEST + "/MI_PlayerShip"


def build_material(textures):
    """
    A parameterised PBR material for the hull.

    Everything is a parameter so the look can be tuned on a Material Instance
    without recompiling the graph - M8 is a tuning pass and recompiling a
    4096-map shader for every tweak is not a workflow.

    The one non-obvious part is the emissive. The albedo is a dark charcoal hull
    with saturated cyan and magenta accent panels painted into it, and unlit
    those panels read as slightly-off grey. Chroma - how far a pixel is from
    neutral - isolates exactly those panels without needing a hand-authored
    emissive mask, so the trim glows and the hull does not.
    """
    if EAL.does_asset_exist(MAT_PATH):
        EAL.delete_asset(MAT_PATH)

    material = AT.create_asset("M_PlayerShip", DEST, unreal.Material,
                               unreal.MaterialFactoryNew())

    def expr(cls, x, y):
        return MEL.create_material_expression(material, cls, x, y)

    def tex_param(name, texture, x, y, sampler=None):
        node = expr(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        node.set_editor_property("parameter_name", name)
        if texture:
            node.set_editor_property("texture", texture)
        if sampler:
            node.set_editor_property("sampler_type", sampler)
        return node

    def scalar(name, value, x, y):
        node = expr(unreal.MaterialExpressionScalarParameter, x, y)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", value)
        return node

    def connect(src, src_out, dst, dst_in):
        MEL.connect_material_expressions(src, src_out, dst, dst_in)

    # ---- Maps ---------------------------------------------------------------
    base = tex_param("BaseColorMap", textures.get("BaseColor"), -1100, -400)
    normal = tex_param("NormalMap", textures.get("Normal"), -1100, 100,
                       unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    rough = tex_param("RoughnessMap", textures.get("Roughness"), -1100, 400,
                      unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    metal = tex_param("MetallicMap", textures.get("Metallic"), -1100, 700,
                      unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)

    # ---- Base colour --------------------------------------------------------
    tint = expr(unreal.MaterialExpressionVectorParameter, -700, -600)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    tinted = expr(unreal.MaterialExpressionMultiply, -400, -450)
    connect(base, "RGB", tinted, "A")
    connect(tint, "RGB", tinted, "B")
    MEL.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # ---- Normal -------------------------------------------------------------
    MEL.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    # ---- Roughness and metallic --------------------------------------------
    rough_scale = scalar("RoughnessScale", 1.0, -700, 500)
    rough_mul = expr(unreal.MaterialExpressionMultiply, -400, 420)
    connect(rough, "R", rough_mul, "A")
    connect(rough_scale, "", rough_mul, "B")
    MEL.connect_material_property(rough_mul, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_scale = scalar("MetallicScale", 1.0, -700, 800)
    metal_mul = expr(unreal.MaterialExpressionMultiply, -400, 720)
    connect(metal, "R", metal_mul, "A")
    connect(metal_scale, "", metal_mul, "B")
    MEL.connect_material_property(metal_mul, "", unreal.MaterialProperty.MP_METALLIC)

    # ---- Emissive from the painted-in neon trim -----------------------------
    # chroma = max(R,G,B) - min(R,G,B). Near zero on the charcoal hull, high on
    # the cyan and magenta panels.
    def channel(mask_r, mask_g, mask_b, y):
        node = expr(unreal.MaterialExpressionComponentMask, -800, y)
        node.set_editor_property("r", mask_r)
        node.set_editor_property("g", mask_g)
        node.set_editor_property("b", mask_b)
        node.set_editor_property("a", False)
        connect(base, "RGB", node, "")
        return node

    red = channel(True, False, False, -1000)
    green = channel(False, True, False, -900)
    blue = channel(False, False, True, -800)

    max_rg = expr(unreal.MaterialExpressionMax, -600, -1000)
    connect(red, "", max_rg, "A")
    connect(green, "", max_rg, "B")

    max_rgb = expr(unreal.MaterialExpressionMax, -450, -1000)
    connect(max_rg, "", max_rgb, "A")
    connect(blue, "", max_rgb, "B")

    min_rg = expr(unreal.MaterialExpressionMin, -600, -830)
    connect(red, "", min_rg, "A")
    connect(green, "", min_rg, "B")

    min_rgb = expr(unreal.MaterialExpressionMin, -450, -830)
    connect(min_rg, "", min_rgb, "A")
    connect(blue, "", min_rgb, "B")

    chroma = expr(unreal.MaterialExpressionSubtract, -300, -900)
    connect(max_rgb, "", chroma, "A")
    connect(min_rgb, "", chroma, "B")

    accent_boost = scalar("AccentBoost", 6.0, -450, -700)
    boosted = expr(unreal.MaterialExpressionMultiply, -150, -880)
    connect(chroma, "", boosted, "A")
    connect(accent_boost, "", boosted, "B")

    # Clamped, or a bright accent would blow the mask past 1 and bleed onto the
    # surrounding hull.
    mask = expr(unreal.MaterialExpressionClamp, 0, -880)
    connect(boosted, "", mask, "")

    accent_colour = expr(unreal.MaterialExpressionMultiply, 150, -800)
    connect(base, "RGB", accent_colour, "A")
    connect(mask, "", accent_colour, "B")

    emissive_strength = scalar("EmissiveStrength", 3.0, 0, -650)
    emissive = expr(unreal.MaterialExpressionMultiply, 350, -750)
    connect(accent_colour, "", emissive, "A")
    connect(emissive_strength, "", emissive, "B")
    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    MEL.recompile_material(material)
    EAL.save_loaded_asset(material)
    log("built material " + MAT_PATH)

    return material


def build_instance(material):
    """The tunable surface. Parameters live here so the graph is left alone."""
    if EAL.does_asset_exist(MI_PATH):
        EAL.delete_asset(MI_PATH)

    factory = unreal.MaterialInstanceConstantFactoryNew()
    instance = AT.create_asset("MI_PlayerShip", DEST,
                               unreal.MaterialInstanceConstant, factory)
    MEL.set_material_instance_parent(instance, material)

    EAL.save_loaded_asset(instance)
    log("built material instance " + MI_PATH)
    return instance


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
    surface = build_instance(material) if material else None

    if surface:
        mesh.set_editor_property("static_materials",
                                 [unreal.StaticMaterial(material_interface=surface)])
        EAL.save_loaded_asset(mesh)

    scale, size, axis = fit_to_hull(mesh)
    wire_to_aircraft(mesh, surface, scale, size, axis)

    log("done")


main()
