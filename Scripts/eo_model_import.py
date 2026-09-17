"""
Shared model import pipeline for the Tripo-generated assets in Raw/.

Every model in this project arrives the same way: one .fbx beside a .fbm folder
of JPEGs, with no authored materials. So the work is always the same - import the
mesh, import the maps, build a parameterised PBR material, wrap it in an instance
to tune on - and only the wiring at the end differs per model.

Used by import_playership.py, import_pistol.py and import_cockpit.py.
"""
import os
import unreal

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

PROJECT = unreal.Paths.project_dir()


def log(tag, msg):
    unreal.log_warning("[{}] {}".format(tag, msg))


def find_source(raw_dir):
    """
    Locates the .fbx in raw_dir and classifies the textures beside it.

    Tripo is inconsistent about naming. Sometimes the maps say what they are
    ("rgb", "normal"); sometimes they are numbered variants of the same export id,
    where _0_0 is albedo and _0_3 is the normal map. Both forms appear in this
    project, so both are handled rather than renaming files by hand.
    """
    fbx = None
    for name in os.listdir(raw_dir):
        if name.lower().endswith(".fbx"):
            fbx = os.path.join(raw_dir, name)
            break

    if not fbx:
        raise RuntimeError("no .fbx found in " + raw_dir)

    textures = {}
    fbm = fbx[:-4] + ".fbm"
    if os.path.isdir(fbm):
        for name in os.listdir(fbm):
            lower = name.lower()
            path = os.path.join(fbm, name)

            if "rgb" in lower or "basecolor" in lower or "albedo" in lower:
                textures["BaseColor"] = path
            elif "normal" in lower:
                textures["Normal"] = path
            elif "rough" in lower:
                textures["Roughness"] = path
            elif "metal" in lower:
                textures["Metallic"] = path
            elif lower.endswith("_0_0.jpg"):
                textures["BaseColor"] = path
            elif lower.endswith("_0_3.jpg"):
                textures["Normal"] = path

    return fbx, textures


def import_mesh(fbx, dest, name, tag):
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
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)

    AT.import_asset_tasks([task])

    path = dest + "/" + name
    mesh = unreal.load_asset(path)
    if not mesh:
        raise RuntimeError("mesh import produced nothing at " + path)

    log(tag, "imported mesh " + path)
    return mesh


def import_texture(path, slot, tex_dest, prefix, tag):
    name = "T_{}_{}".format(prefix, slot)
    asset_path = tex_dest + "/" + name

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", tex_dest)
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
    log(tag, "imported texture " + asset_path)
    return texture


def import_textures(textures, tex_dest, prefix, tag):
    return {slot: import_texture(path, slot, tex_dest, prefix, tag)
            for slot, path in textures.items()}


def build_material(textures, dest, name, tag, accent_boost=6.0, emissive_strength=3.0):
    """
    A parameterised PBR material.

    Everything is a parameter so the look can be tuned on a Material Instance
    without recompiling the graph - M8 is a tuning pass and recompiling a
    4096-map shader for every tweak is not a workflow.

    The one non-obvious part is the emissive. These albedos are dark charcoal
    with saturated cyan and magenta accents painted in, and unlit those accents
    read as slightly-off grey. Chroma - how far a pixel is from neutral -
    isolates exactly those accents without a hand-authored emissive mask, so the
    trim glows and the body does not.
    """
    path = dest + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    material = AT.create_asset(name, dest, unreal.Material, unreal.MaterialFactoryNew())

    def expr(cls, x, y):
        return MEL.create_material_expression(material, cls, x, y)

    def tex_param(param, texture, x, y, sampler=None):
        node = expr(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        node.set_editor_property("parameter_name", param)
        if texture:
            node.set_editor_property("texture", texture)
        if sampler:
            node.set_editor_property("sampler_type", sampler)
        return node

    def scalar(param, value, x, y):
        node = expr(unreal.MaterialExpressionScalarParameter, x, y)
        node.set_editor_property("parameter_name", param)
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
    if textures.get("Normal"):
        MEL.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    # ---- Roughness and metallic --------------------------------------------
    if textures.get("Roughness"):
        rough_scale = scalar("RoughnessScale", 1.0, -700, 500)
        rough_mul = expr(unreal.MaterialExpressionMultiply, -400, 420)
        connect(rough, "R", rough_mul, "A")
        connect(rough_scale, "", rough_mul, "B")
        MEL.connect_material_property(rough_mul, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if textures.get("Metallic"):
        metal_scale = scalar("MetallicScale", 1.0, -700, 800)
        metal_mul = expr(unreal.MaterialExpressionMultiply, -400, 720)
        connect(metal, "R", metal_mul, "A")
        connect(metal_scale, "", metal_mul, "B")
        MEL.connect_material_property(metal_mul, "", unreal.MaterialProperty.MP_METALLIC)

    # ---- Emissive from the painted-in neon trim -----------------------------
    # chroma = max(R,G,B) - min(R,G,B). Near zero on charcoal, high on the
    # cyan and magenta accents.
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

    boost = scalar("AccentBoost", accent_boost, -450, -700)
    boosted = expr(unreal.MaterialExpressionMultiply, -150, -880)
    connect(chroma, "", boosted, "A")
    connect(boost, "", boosted, "B")

    # Clamped, or a bright accent blows the mask past 1 and bleeds onto the
    # surrounding surface.
    mask = expr(unreal.MaterialExpressionClamp, 0, -880)
    connect(boosted, "", mask, "")

    accent_colour = expr(unreal.MaterialExpressionMultiply, 150, -800)
    connect(base, "RGB", accent_colour, "A")
    connect(mask, "", accent_colour, "B")

    strength = scalar("EmissiveStrength", emissive_strength, 0, -650)
    emissive = expr(unreal.MaterialExpressionMultiply, 350, -750)
    connect(accent_colour, "", emissive, "A")
    connect(strength, "", emissive, "B")
    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    MEL.recompile_material(material)
    EAL.save_loaded_asset(material)
    log(tag, "built material " + path)

    return material


def build_instance(material, dest, name, tag):
    """The tunable surface. Parameters live here so the graph is left alone."""
    path = dest + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    factory = unreal.MaterialInstanceConstantFactoryNew()
    instance = AT.create_asset(name, dest, unreal.MaterialInstanceConstant, factory)
    MEL.set_material_instance_parent(instance, material)

    EAL.save_loaded_asset(instance)
    log(tag, "built material instance " + path)
    return instance


def assign_material(mesh, surface):
    mesh.set_editor_property("static_materials",
                             [unreal.StaticMaterial(material_interface=surface)])
    EAL.save_loaded_asset(mesh)


def measure(mesh):
    """Returns (size, longest_axis_name) in centimetres."""
    extent = mesh.get_bounds().box_extent
    size = unreal.Vector(extent.x * 2.0, extent.y * 2.0, extent.z * 2.0)

    longest = max(size.x, size.y, size.z)
    if longest <= 0.0:
        raise RuntimeError("mesh has no size")

    axis = "X" if longest == size.x else ("Y" if longest == size.y else "Z")
    return size, axis, longest


def fit_scale(mesh, target_length, tag):
    """Uniform scale that makes the mesh's longest axis target_length cm."""
    size, axis, longest = measure(mesh)
    scale = target_length / longest

    log(tag, "mesh size {:.1f} x {:.1f} x {:.1f}cm -> uniform scale {:.4f} (longest axis {})"
        .format(size.x, size.y, size.z, scale, axis))

    return scale, size, axis


def import_model(raw_name, dest, prefix, tag, target_length):
    """
    The whole common path: source -> mesh + textures -> material -> instance.

    Returns (mesh, surface, scale, size, axis).
    """
    raw_dir = os.path.join(PROJECT, "Raw", raw_name)

    fbx, texture_paths = find_source(raw_dir)
    log(tag, "source: " + os.path.basename(fbx))
    log(tag, "maps: " + (", ".join(sorted(texture_paths.keys())) or "none"))

    mesh = import_mesh(fbx, dest, "SM_" + prefix, tag)
    textures = import_textures(texture_paths, dest + "/Textures", prefix, tag)

    material = build_material(textures, dest, "M_" + prefix, tag)
    surface = build_instance(material, dest, "MI_" + prefix, tag)
    assign_material(mesh, surface)

    scale, size, axis = fit_scale(mesh, target_length, tag)
    return mesh, surface, scale, size, axis
