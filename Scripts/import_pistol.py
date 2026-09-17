"""
Imports the player pistol and hangs it on BP_Operative.

Run headless:

    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/import_pistol.py"

Source art lives in Raw/PlayerPistol (gitignored); this produces the committed
assets under /Game/Weapons/PlayerPistol. Idempotent.

The carry logic is in C++ (AEOOperativeCharacter): the pistol sits on the right
thigh until the operative aims or fires, then moves to the right hand. This
script only supplies the mesh and a sensible scale.
"""
import os
import sys

import unreal

sys.path.append(os.path.join(unreal.Paths.project_dir(), "Scripts"))

import eo_model_import as eo

TAG = "Pistol"
DEST = "/Game/Weapons/PlayerPistol"
BP_OPERATIVE = "/Game/Blueprints/BP_Operative"

# A sidearm, measured along its longest axis: barrel tip to the back of the
# grip. 24cm is a large service pistol, which is what the model reads as.
TARGET_LENGTH = 24.0


def wire_to_operative(mesh, surface, scale, axis):
    blueprint = unreal.load_asset(BP_OPERATIVE)
    if not blueprint:
        raise RuntimeError("BP_Operative not found; run m0_setup.py first")

    cdo = unreal.get_default_object(blueprint.generated_class())
    pistol = cdo.get_editor_property("PistolMesh")

    pistol.set_editor_property("static_mesh", mesh)
    pistol.set_editor_property("relative_scale3d", unreal.Vector(scale, scale, scale))

    if surface:
        pistol.set_material(0, surface)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    eo.EAL.save_loaded_asset(blueprint)

    eo.log(TAG, "wired onto BP_Operative (longest axis {}, scale {:.4f})".format(axis, scale))
    eo.log(TAG, "carry offsets are tunable on the Blueprint: Combat|Weapon > "
                "HolsterOffset / HolsterRotation / GripOffset / GripRotation")


def main():
    eo.log(TAG, "importing player pistol")

    mesh, surface, scale, _size, axis = eo.import_model(
        "PlayerPistol", DEST, "PlayerPistol", TAG, TARGET_LENGTH)

    wire_to_operative(mesh, surface, scale, axis)
    eo.log(TAG, "done")


main()
