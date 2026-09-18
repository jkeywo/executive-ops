"""
Builds the Enhanced Input assets: one Input Action per verb, one Input Mapping
Context per pawn, and the DA_EOInputConfig data asset that ties them together.

This script is the source of truth and the assets are its output, the same
arrangement as the feedback presets. The bindings used to be constructed in
C++ on every run, which kept them readable in a diff but meant nothing set on
the object in the editor could ever survive. Keeping the table here keeps the
diff; making the output an asset makes it real. See Docs/adr/0001 and 0008.

Idempotent: existing assets are updated in place, so re-running after a change
to the table below is the whole workflow.

Run with the editor closed:

    UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript \
        -script="Scripts/build_input_assets.py"
"""

import unreal

ROOT = "/Game/Input"
ACTIONS_PATH = ROOT + "/Actions"
CONFIG_NAME = "DA_EOInputConfig"

# Value types, matching what each handler reads out of the FInputActionValue.
AXIS2D = unreal.InputActionValueType.AXIS2D
AXIS1D = unreal.InputActionValueType.AXIS1D
BOOLEAN = unreal.InputActionValueType.BOOLEAN

# name -> (value type, property on UEOInputConfig)
ACTIONS = {
    "IA_Look":           (AXIS2D,  "look_action"),
    "IA_LookStick":      (AXIS2D,  "look_stick_action"),
    "IA_FlightMove":     (AXIS2D,  "flight_move_action"),
    "IA_FlightVertical": (AXIS1D,  "flight_vertical_action"),
    "IA_FlightYaw":      (AXIS1D,  "flight_yaw_action"),
    "IA_Hover":          (BOOLEAN, "hover_action"),
    "IA_Deploy":         (BOOLEAN, "deploy_action"),
    "IA_ToggleMap":      (BOOLEAN, "toggle_map_action"),
    "IA_ToggleView":     (BOOLEAN, "toggle_view_action"),
    "IA_Move":           (AXIS2D,  "move_action"),
    "IA_Jump":           (BOOLEAN, "jump_action"),
    "IA_Sprint":         (BOOLEAN, "sprint_action"),
    "IA_Slide":          (BOOLEAN, "slide_action"),
    "IA_Takedown":       (BOOLEAN, "takedown_action"),
    "IA_Fire":           (BOOLEAN, "fire_action"),
    "IA_Aim":            (BOOLEAN, "aim_action"),
    "IA_Interact":       (BOOLEAN, "interact_action"),
}

# Modifier shorthands. NEG flips every axis; SWZ swaps X and Y so a 1D key
# feeds the Y of a 2D action - W/S drive forward/back, A/D drive left/right.
NEG = "negate"
SWZ = "swizzle"

# (action, key, modifiers). Same table as the C++ it replaces, row for row.
# Look inversion is deliberately absent: it is applied where the value is
# consumed, from UEOInputSettings, so changing it does not need a rebuild.
AIRCRAFT = [
    # WASD translates, Space/Ctrl climbs and descends, Q/E yaws, Shift holds hover.
    ("IA_FlightMove",     "W",                         [SWZ]),
    ("IA_FlightMove",     "S",                         [SWZ, NEG]),
    ("IA_FlightMove",     "A",                         [NEG]),
    ("IA_FlightMove",     "D",                         []),
    ("IA_FlightVertical", "SpaceBar",                  []),
    ("IA_FlightVertical", "LeftControl",               [NEG]),
    ("IA_FlightYaw",      "E",                         []),
    ("IA_FlightYaw",      "Q",                         [NEG]),
    ("IA_Hover",          "LeftShift",                 []),
    ("IA_Deploy",         "F",                         []),
    ("IA_ToggleMap",      "M",                         []),
    ("IA_ToggleView",     "V",                         []),
    ("IA_Look",           "Mouse2D",                   []),
    # Gamepad mirrors the keyboard rather than inventing a second scheme.
    ("IA_FlightMove",     "Gamepad_Left2D",            []),
    ("IA_FlightVertical", "Gamepad_RightTriggerAxis",  []),
    ("IA_FlightVertical", "Gamepad_LeftTriggerAxis",   [NEG]),
    ("IA_FlightYaw",      "Gamepad_RightShoulder",     []),
    ("IA_FlightYaw",      "Gamepad_LeftShoulder",      [NEG]),
    ("IA_Hover",          "Gamepad_FaceButton_Left",   []),
    ("IA_Deploy",         "Gamepad_FaceButton_Bottom", []),
    ("IA_ToggleMap",      "Gamepad_FaceButton_Top",    []),
    ("IA_ToggleView",     "Gamepad_RightThumbstick",   []),
    ("IA_LookStick",      "Gamepad_Right2D",           []),
]

OPERATIVE = [
    ("IA_Move",      "W",                         [SWZ]),
    ("IA_Move",      "S",                         [SWZ, NEG]),
    ("IA_Move",      "A",                         [NEG]),
    ("IA_Move",      "D",                         []),
    ("IA_Jump",      "SpaceBar",                  []),
    ("IA_Sprint",    "LeftShift",                 []),
    # Jump is the contextual parkour button; the decision lives in one handler.
    ("IA_Slide",     "LeftControl",               []),
    ("IA_Slide",     "C",                         []),
    ("IA_Takedown",  "F",                         []),
    ("IA_Fire",      "LeftMouseButton",           []),
    ("IA_Aim",       "RightMouseButton",          []),
    ("IA_Interact",  "E",                         []),
    ("IA_Look",      "Mouse2D",                   []),
    ("IA_Move",      "Gamepad_Left2D",            []),
    ("IA_Jump",      "Gamepad_FaceButton_Bottom", []),
    ("IA_Slide",     "Gamepad_FaceButton_Right",  []),
    ("IA_Takedown",  "Gamepad_FaceButton_Top",    []),
    ("IA_Interact",  "Gamepad_FaceButton_Left",   []),
    ("IA_Sprint",    "Gamepad_LeftThumbstick",    []),
    ("IA_Aim",       "Gamepad_LeftTrigger",       []),
    ("IA_Fire",      "Gamepad_RightTrigger",      []),
    ("IA_LookStick", "Gamepad_Right2D",           []),
]

CONTEXTS = {
    "IMC_Aircraft":  ("aircraft_context",  AIRCRAFT),
    "IMC_Operative": ("operative_context", OPERATIVE),
}


def log(message):
    # Display-level Python logging is filtered under -unattended; warning always
    # shows, which is the convention the rest of the scripts in here follow.
    unreal.log_warning("[input] {}".format(message))


def load_or_create(path, name, asset_class, factory):
    full = "{}/{}".format(path, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        return unreal.EditorAssetLibrary.load_asset(full)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    return tools.create_asset(name, path, asset_class, factory)


def make_key(name):
    # Key's name is read-only from Python, as GameplayTag's was; import_text is
    # the route the struct exposes.
    key = unreal.Key()
    key.import_text(name)
    return key


def make_modifier(kind, outer):
    if kind == NEG:
        modifier = unreal.new_object(unreal.InputModifierNegate, outer=outer)
        modifier.set_editor_property("x", True)
        modifier.set_editor_property("y", True)
        modifier.set_editor_property("z", True)
        return modifier
    if kind == SWZ:
        modifier = unreal.new_object(unreal.InputModifierSwizzleAxis, outer=outer)
        modifier.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
        return modifier
    raise ValueError(kind)


def build_actions():
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.InputAction)
    actions = {}
    for name, (value_type, _) in ACTIONS.items():
        action = load_or_create(ACTIONS_PATH, name, unreal.InputAction, factory)
        action.set_editor_property("value_type", value_type)
        unreal.EditorAssetLibrary.save_asset("{}/{}".format(ACTIONS_PATH, name))
        actions[name] = action
    log("{} actions".format(len(actions)))
    return actions


def build_context(name, rows, actions):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.InputMappingContext)
    context = load_or_create(ROOT, name, unreal.InputMappingContext, factory)

    # Built as structs and assigned in one go. The context's map_key is exposed
    # to Python but hands back a copy and never touches the array, so a script
    # that calls it ends up with a context holding nothing - which was the first
    # thing this script did. Assigning the array is the route that persists.
    #
    # Rebuilt from the table every run, so a row removed here is a binding
    # removed there - the point of the script being the source of truth.
    mappings = []
    for action_name, key_name, modifiers in rows:
        mapping = unreal.EnhancedActionKeyMapping()
        mapping.set_editor_property("action", actions[action_name])
        mapping.set_editor_property("key", make_key(key_name))
        mapping.set_editor_property(
            "modifiers", [make_modifier(kind, context) for kind in modifiers])
        mappings.append(mapping)
    context.set_editor_property("mappings", mappings)

    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ROOT, name))
    log("{}: {} bindings".format(name, len(rows)))
    return context


def build_config(actions, contexts):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.EOInputConfig)
    config = load_or_create(ROOT, CONFIG_NAME, unreal.EOInputConfig, factory)

    for name, (_, prop) in ACTIONS.items():
        config.set_editor_property(prop, actions[name])
    for name, (prop, _) in CONTEXTS.items():
        config.set_editor_property(prop, contexts[name])

    unreal.EditorAssetLibrary.save_asset("{}/{}".format(ROOT, CONFIG_NAME))
    log("saved {}".format(CONFIG_NAME))


def main():
    actions = build_actions()
    contexts = {name: build_context(name, rows, actions)
                for name, (_, rows) in CONTEXTS.items()}
    build_config(actions, contexts)


main()
