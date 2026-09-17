"""Reports what the ship's mesh, material and textures actually contain."""
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

MESH_PATH = "/Game/Vehicles/PlayerShip/SM_PlayerShip"
MAT_PATH = "/Game/Vehicles/PlayerShip/M_PlayerShip"
TEX_DEST = "/Game/Vehicles/PlayerShip/Textures"


def log(msg):
    unreal.log_warning("[Verify] " + msg)


mesh = unreal.load_asset(MESH_PATH)
log("mesh: " + ("found" if mesh else "MISSING"))

if mesh:
    slots = mesh.get_editor_property("static_materials")
    log("material slots: {}".format(len(slots)))
    for i, slot in enumerate(slots):
        mi = slot.get_editor_property("material_interface")
        log("  slot {}: {}".format(i, mi.get_path_name() if mi else "NONE"))

material = unreal.load_asset(MAT_PATH)
log("material: " + ("found" if material else "MISSING"))

if material:
    props = [
        ("BaseColor", unreal.MaterialProperty.MP_BASE_COLOR),
        ("Metallic", unreal.MaterialProperty.MP_METALLIC),
        ("Roughness", unreal.MaterialProperty.MP_ROUGHNESS),
        ("Normal", unreal.MaterialProperty.MP_NORMAL),
        ("Emissive", unreal.MaterialProperty.MP_EMISSIVE_COLOR),
    ]
    for name, prop in props:
        try:
            connected = MEL.get_material_property_input_node(material, prop)
        except Exception:
            connected = None
        log("  {} <- {}".format(name, connected.get_name() if connected else "nothing"))

    expressions = MEL.get_material_expressions(material) if hasattr(
        MEL, "get_material_expressions") else []
    log("  expressions: {}".format(len(expressions)))

for slot in ("BaseColor", "Normal", "Roughness", "Metallic"):
    path = TEX_DEST + "/T_PlayerShip_" + slot
    texture = unreal.load_asset(path)
    if not texture:
        log("texture {}: MISSING".format(slot))
        continue
    log("texture {}: {}x{} srgb={} compression={}".format(
        slot,
        texture.blueprint_get_size_x(), texture.blueprint_get_size_y(),
        texture.get_editor_property("srgb"),
        texture.get_editor_property("compression_settings")))
