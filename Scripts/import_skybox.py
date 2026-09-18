"""
Imports Raw/Skybox.png as an equirectangular skybox and places it in the maps.

The engine ships a sky sphere mesh (SM_SkySphere) whose UVs are already
equirectangular - it is what BP_Sky_Sphere's star layer is mapped onto. So a
panorama becomes a skybox with no geometry of our own: an unlit material that
samples the panorama through the mesh UVs, flagged as sky so height fog and the
atmosphere leave it alone, on that mesh scaled out to the world edge.

Idempotent by label, like place_functional_tests.py: a re-run re-imports the
texture, rebuilds the material and updates the placed actor rather than
duplicating it. The SkyLight in each map real-time captures, so it picks the
panorama up as ambient light on the next launch without further work.

Run with the editor closed:

    UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript \
        -script="C:/Coding/executive-ops/Scripts/import_skybox.py"
"""

import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import eo_editor  # noqa: E402

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

SOURCE = os.path.join(unreal.Paths.project_dir(), "Raw", "Skybox.png")
DEST = "/Game/Environment/Sky"
TEXTURE_NAME = "T_Skybox"
MATERIAL_NAME = "M_Skybox"
INSTANCE_NAME = "MI_Skybox"
SKY_MESH = "/Engine/EngineSky/SM_SkySphere"
ACTOR_LABEL = "Skybox"
MAPS = ["/Game/Maps/L_FlightTest", "/Game/Maps/L_MissionTest"]

# How far out the sphere sits. Well past anything the flight map reaches,
# well inside the engine's world bounds.
RADIUS = 1000000.0


def log(message):
    unreal.log_warning("[skybox] {}".format(message))


def import_texture():
    if not os.path.isfile(SOURCE):
        raise RuntimeError("no panorama at " + SOURCE)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", TEXTURE_NAME)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    AT.import_asset_tasks([task])

    texture = unreal.load_asset(DEST + "/" + TEXTURE_NAME)
    if not texture:
        raise RuntimeError("texture import failed: " + SOURCE)

    # A panorama stretches around the whole view: no mip streaming pop-in, and
    # no power-of-two padding distorting the 2:1 mapping.
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.NONE)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    EAL.save_loaded_asset(texture)
    log("imported " + DEST + "/" + TEXTURE_NAME)
    return texture


def build_material(texture):
    path = DEST + "/" + MATERIAL_NAME
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)

    material = AT.create_asset(MATERIAL_NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    # The engine's own sky treatment: excluded from height fog and aerial
    # perspective, so the panorama reads at full contrast at any distance.
    material.set_editor_property("is_sky", True)

    sample = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
    sample.set_editor_property("parameter_name", "Panorama")
    sample.set_editor_property("texture", texture)

    brightness = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 250)
    brightness.set_editor_property("parameter_name", "Brightness")
    brightness.set_editor_property("default_value", 1.0)

    multiply = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 100)
    MEL.connect_material_expressions(sample, "RGB", multiply, "A")
    MEL.connect_material_expressions(brightness, "", multiply, "B")
    MEL.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    MEL.recompile_material(material)
    EAL.save_loaded_asset(material)
    log("built " + path)
    return material


def build_instance(material):
    path = DEST + "/" + INSTANCE_NAME
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    instance = AT.create_asset(INSTANCE_NAME, DEST, unreal.MaterialInstanceConstant,
                               unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(instance, material)
    EAL.save_loaded_asset(instance)
    log("built " + path)
    return instance


def existing(actors, label):
    for actor in actors:
        if actor.get_actor_label() == label:
            return actor
    return None


def place(map_path, mesh, surface):
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level_subsystem.load_level(map_path)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = existing(actor_subsystem.get_all_level_actors(), ACTOR_LABEL)
    if actor is None:
        actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
        actor.set_actor_label(ACTOR_LABEL)
        log("placed {} in {}".format(ACTOR_LABEL, map_path))
    else:
        log("updated {} in {}".format(ACTOR_LABEL, map_path))

    actor.set_mobility(unreal.ComponentMobility.STATIC)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_material(0, surface)
    component.set_editor_property("cast_shadow", False)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    extent = mesh.get_bounds().sphere_radius
    scale = RADIUS / extent
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))

    if not eo_editor.save_current_level(level_subsystem, actor_subsystem):
        raise RuntimeError("could not save " + map_path)


def main():
    mesh = unreal.load_asset(SKY_MESH)
    if not mesh:
        raise RuntimeError("engine sky sphere missing: " + SKY_MESH)

    texture = import_texture()
    material = build_material(texture)
    instance = build_instance(material)
    for map_path in MAPS:
        place(map_path, mesh, instance)
    log("done")


main()
