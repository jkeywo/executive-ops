"""
Imports the player ship model and wires it onto BP_Aircraft.

Run headless:

    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/import_playership.py"

Source art lives in Raw/PlayerShip (gitignored); this produces the committed
assets under /Game/Vehicles/PlayerShip. Idempotent: re-running reimports and
re-wires.

The mesh, texture and material work is shared with the pistol and cockpit - see
eo_model_import.py. Only the wiring below is specific to the aircraft.
"""
import os
import sys

import unreal

sys.path.append(os.path.join(unreal.Paths.project_dir(), "Scripts"))

import eo_model_import as eo

TAG = "Ship"
DEST = "/Game/Vehicles/PlayerShip"
BP_AIRCRAFT = "/Game/Blueprints/BP_Aircraft"

# The hull should read at roughly the size the collision box already assumes:
# 800 x 600 x 240cm. Fitting to the longest axis keeps the proportions honest.
TARGET_LENGTH = 800.0

# Which way a Tripo export faces cannot be derived from its bounds - the
# heuristic that used to live here ("longest axis is forward") guessed wrong for
# the cockpit, and put the player looking out of the right-hand window. These
# values were each established by rendering the model and looking at it, and
# should only be changed the same way.
#
# Verified from a side-on capture: the nose points down +X, matching every pawn
# in the project.
MODEL_YAW = -90.0


def wire_to_aircraft(mesh, surface, scale, size):
    blueprint = unreal.load_asset(BP_AIRCRAFT)
    if not blueprint:
        raise RuntimeError("BP_Aircraft not found; run m0_setup.py first")

    cdo = unreal.get_default_object(blueprint.generated_class())
    hull = cdo.get_editor_property("HullMesh")

    hull.set_editor_property("static_mesh", mesh)
    hull.set_editor_property("relative_scale3d", unreal.Vector(scale, scale, scale))

    hull.set_editor_property("relative_rotation",
                             unreal.Rotator(roll=0.0, pitch=0.0, yaw=MODEL_YAW))

    if surface:
        hull.set_material(0, surface)

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
    # The yaw above swaps which model axis lies along the pawn's X.
    scaled = [size.x * scale, size.y * scale, size.z * scale]
    if abs(MODEL_YAW) == 90.0:
        scaled[0], scaled[1] = scaled[1], scaled[0]

    collision = cdo.get_editor_property("CollisionBox")
    collision.set_editor_property("box_extent", unreal.Vector(
        scaled[0] * 0.48, scaled[1] * 0.46, scaled[2] * 0.32))
    eo.log(TAG, "collision extents set to {:.0f} x {:.0f} x {:.0f}".format(
        scaled[0] * 0.48, scaled[1] * 0.46, scaled[2] * 0.32))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    eo.EAL.save_loaded_asset(blueprint)
    eo.log(TAG, "wired onto BP_Aircraft (yaw {:.0f})".format(MODEL_YAW))


def main():
    eo.log(TAG, "importing player ship")

    mesh, surface, scale, size, _axis = eo.import_model(
        "PlayerShip", DEST, "PlayerShip", TAG, TARGET_LENGTH)

    wire_to_aircraft(mesh, surface, scale, size)
    eo.log(TAG, "done")


main()
