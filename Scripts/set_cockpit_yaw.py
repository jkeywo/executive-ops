"""
Sets the cockpit mesh's yaw on BP_Aircraft, for finding the correct one by eye.

    set EO_COCKPIT_YAW=90
    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/set_cockpit_yaw.py"

Which way a Tripo export faces is not knowable from its bounds, so the only
reliable way to orient an interior is to look through it.
"""
import os

import unreal

BP_AIRCRAFT = "/Game/Blueprints/BP_Aircraft"

yaw = float(os.environ.get("EO_COCKPIT_YAW", "0"))

blueprint = unreal.load_asset(BP_AIRCRAFT)
cdo = unreal.get_default_object(blueprint.generated_class())

cockpit = cdo.get_editor_property("CockpitMesh")
current = cockpit.get_editor_property("relative_rotation")

cockpit.set_editor_property("relative_rotation",
                            unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

unreal.log_warning("[Cockpit] yaw {:.0f} -> {:.0f}".format(current.yaw, yaw))
