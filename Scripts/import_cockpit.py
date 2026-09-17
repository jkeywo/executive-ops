"""
Imports the cockpit interior and seats it in BP_Aircraft.

Run headless:

    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/import_cockpit.py"

Source art lives in Raw/PlayerCockpit (gitignored); this produces the committed
assets under /Game/Vehicles/PlayerCockpit. Idempotent.

The view toggle itself is in C++ (AEOAircraftPawn): V swaps between the cockpit
camera and the chase boom, first person by default. This script places the
interior and the pilot's viewpoint inside the hull.
"""
import os
import sys

import unreal

sys.path.append(os.path.join(unreal.Paths.project_dir(), "Scripts"))

import eo_model_import as eo

TAG = "Cockpit"
DEST = "/Game/Vehicles/PlayerCockpit"
BP_AIRCRAFT = "/Game/Blueprints/BP_Aircraft"

# The hull is fitted to 800cm long. A cockpit tub is roughly a quarter of that,
# which puts the canopy frame at the edges of view rather than in the middle of
# it.
TARGET_LENGTH = 200.0

# Where the tub sits inside the hull: forward of centre, and up to the height a
# seated pilot's eyeline would actually be.
COCKPIT_FORWARD = 150.0
COCKPIT_UP = 40.0

# The viewpoint inside that tub.
#
# The mesh's pivot is centred in plan and sits on the cockpit floor, so these are
# measured from the floor up: a seated pilot's eyeline is a little over half the
# height of the tub, and slightly aft of centre so the console is ahead of them
# rather than around them.
EYE_FORWARD = 5.0
EYE_UP = 94.0


def wire_to_aircraft(mesh, surface, scale, axis):
    blueprint = unreal.load_asset(BP_AIRCRAFT)
    if not blueprint:
        raise RuntimeError("BP_Aircraft not found; run m0_setup.py first")

    cdo = unreal.get_default_object(blueprint.generated_class())

    pivot = cdo.get_editor_property("CockpitPivot")
    pivot.set_editor_property("relative_location",
                              unreal.Vector(COCKPIT_FORWARD, 0.0, COCKPIT_UP))

    cockpit = cdo.get_editor_property("CockpitMesh")
    cockpit.set_editor_property("static_mesh", mesh)
    cockpit.set_editor_property("relative_scale3d", unreal.Vector(scale, scale, scale))

    # Turn the model so its long axis points down +X, which is forward for every
    # pawn in this project. Tripo exports are not authored to any particular
    # forward.
    yaw = -90.0 if axis == "Y" else 0.0
    cockpit.set_editor_property("relative_rotation",
                                unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))

    if surface:
        cockpit.set_material(0, surface)

    camera = cdo.get_editor_property("CockpitCamera")
    camera.set_editor_property("relative_location",
                               unreal.Vector(EYE_FORWARD, 0.0, EYE_UP))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    eo.EAL.save_loaded_asset(blueprint)

    eo.log(TAG, "wired onto BP_Aircraft (yaw {:.0f}, scale {:.4f})".format(yaw, scale))
    eo.log(TAG, "seat and eyeline are tunable on the Blueprint: CockpitPivot and "
                "CockpitCamera relative locations")


def main():
    eo.log(TAG, "importing cockpit")

    mesh, surface, scale, _size, axis = eo.import_model(
        "PlayerCockpit", DEST, "PlayerCockpit", TAG, TARGET_LENGTH)

    wire_to_aircraft(mesh, surface, scale, axis)
    eo.log(TAG, "done")


main()
