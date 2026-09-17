"""
Report root-motion settings, and the actual root-bone travel, for every clip the
game plays.

Single-node playback does not consume root motion - it just evaluates the pose,
root bone included. A clip with translation or rotation baked into the root
therefore slides or spins the rendered model away from the capsule it belongs
to. This prints the evidence rather than assuming it.

    UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Scripts/inspect_root_motion.py"
"""
import ast
import io
import os

import unreal

# Read the tables out of the setup script rather than importing it: importing
# would run the whole map build as a side effect.
SETUP = r"C:\Coding\executive-ops\Scripts\m0_setup.py"
TREE = ast.parse(io.open(SETUP, encoding="utf-8").read())

CONSTS = {}
TABLES = {}
for node in TREE.body:
    if not isinstance(node, ast.Assign) or not isinstance(node.targets[0], ast.Name):
        continue
    name = node.targets[0].id
    try:
        value = eval(compile(ast.Expression(node.value), "<tables>", "eval"), {}, dict(CONSTS))
    except Exception:
        continue
    CONSTS[name] = value
    if isinstance(value, dict):
        TABLES[name] = value

seen = {}
for table_name, mapping in TABLES.items():
    for prop, path in mapping.items():
        if isinstance(path, str) and path.startswith("/Game/"):
            seen.setdefault(path, []).append(table_name + "." + prop)

print("=== root motion report ===")
for path in sorted(seen):
    anim = unreal.load_asset(path)
    if not anim:
        print("MISSING  " + path)
        continue

    def prop(name):
        try:
            return anim.get_editor_property(name)
        except Exception as exc:
            return "<" + type(exc).__name__ + ">"

    length = prop("sequence_length")
    travel = "n/a"
    try:
        end_time = max(float(length) - 0.001, 0.0)
        start = unreal.AnimationLibrary.get_bone_pose_for_time(anim, "root", 0.0, False)
        end = unreal.AnimationLibrary.get_bone_pose_for_time(anim, "root", end_time, False)
        d = end.translation - start.translation
        travel = "root dx=%7.1f dy=%6.1f dz=%6.1f dyaw=%6.1f" % (
            d.x, d.y, d.z,
            end.rotation.rotator().yaw - start.rotation.rotator().yaw)
    except Exception as exc:
        travel = "<" + type(exc).__name__ + ": " + str(exc) + ">"

    print("%-34s len=%-5s rm=%-5s lock=%-28s force=%-5s %s   [%s]" % (
        path.rsplit("/", 1)[-1],
        ("%.2f" % length) if isinstance(length, float) else length,
        prop("enable_root_motion"),
        prop("root_motion_root_lock"),
        prop("force_root_lock"),
        travel,
        ", ".join(seen[path])))
print("=== end root motion report ===")
