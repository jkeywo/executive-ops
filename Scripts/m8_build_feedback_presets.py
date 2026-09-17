"""
Builds /Game/Feedback/DA_EOFeedbackPresets from whichever asset packs are present.

The C++ never references third-party content directly - it names events, and this
script binds those names to assets. That separation is what lets a clone without
the packs still compile, launch and play, silently.

Run headless:

    UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript \
        -script="Scripts/m8_build_feedback_presets.py"

Every binding below lists candidates in preference order. The first asset that
actually exists wins; if none do, the field is left unset and that layer of the
event does nothing. Re-running after importing more packs fills in the gaps, so
this is safe to run repeatedly.
"""

import unreal

PACKAGE_PATH = "/Game/Feedback"
ASSET_NAME = "DA_EOFeedbackPresets"

# ---------------------------------------------------------------- asset lookup

_asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
_missing = set()


def first_existing(*paths):
    """First asset path that resolves, or None. Records misses for the summary."""
    for path in paths:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.EditorAssetLibrary.load_asset(path)
        _missing.add(path.rsplit("/", 1)[0])
    return None


# Shorthands for the pack roots, so the table below stays readable.
GUN = "/Game/FreeWeaponSounds/Cue"
FOOT = "/Game/Essential_Foosteps_SK/CUE"
VOX = "/Game/HumanVocalizations"
UI = "/Game/SciFiUISFX/Cues"
MUZZLE = "/Game/NW_MuzzleFX/Particle_FX"
NIAGARA = "/Game/NiagaraExamples/FX_Weapons"
BIGFX = "/Game/BigNiagaraBundle"
LYRA = "/Game/Audio/Sounds"


def preset(
    sound=None,
    effect=None,
    shake=None,
    intensity=unreal.EOFeedbackIntensity.NORMAL,
    volume=1.0,
    pitch_jitter=0.0,
    effect_scale=1.0,
    attach_effect=False,
    shake_scale=1.0,
    fov=0.0,
    fov_decay=0.3,
    vignette=0.0,
    vignette_decay=0.6,
    directional=False,
    hit_stop=0.0,
):
    """One FEOFeedbackPreset, with only the layers this event actually needs."""
    p = unreal.EOFeedbackPreset()
    p.set_editor_property("intensity", intensity)
    p.set_editor_property("volume_multiplier", volume)
    p.set_editor_property("pitch_jitter", pitch_jitter)
    p.set_editor_property("effect_scale", effect_scale)
    p.set_editor_property("b_attach_effect", attach_effect)
    p.set_editor_property("camera_shake_scale", shake_scale)
    p.set_editor_property("fov_impulse", fov)
    p.set_editor_property("fov_impulse_decay", fov_decay)
    p.set_editor_property("vignette_intensity", vignette)
    p.set_editor_property("vignette_decay", vignette_decay)
    p.set_editor_property("b_directional_indicator", directional)
    p.set_editor_property("hit_stop_seconds", hit_stop)

    if sound is not None:
        p.set_editor_property("sound", sound)
    if effect is not None:
        p.set_editor_property("effect", effect)
    if shake is not None:
        p.set_editor_property("camera_shake", shake)

    return p


def shake_class(name):
    """Camera shake classes live in C++; look them up by name so a rename is loud."""
    cls = getattr(unreal, name, None)
    if cls is None:
        unreal.log_warning("Camera shake class not found: {}".format(name))
    return cls


def build_presets():
    """
    The whole feedback vocabulary, one entry per event in EOFeedbackEvents.h.

    Sound choices are deliberately restrained. Pistol_Fire gets the shot plus a
    muzzle flash and a recoil kick, not a shot plus flash plus dust plus shake
    plus haptic - the brief's rule is the smallest combination that reads.
    """
    presets = {}

    # ---- Flight -------------------------------------------------------------
    # Nothing owned covers aircraft engines, so these carry camera and FOV only.
    # That is the honest state of the library, and it is still most of what makes
    # speed read - see GDD/executive-ops-m8-asset-sourcing-report.md §H.
    presets["Aircraft_Accelerate"] = preset(
        fov=7.0, fov_decay=0.45, intensity=unreal.EOFeedbackIntensity.NORMAL
    )
    presets["Aircraft_BrakeHard"] = preset(
        fov=-9.0, fov_decay=0.35, intensity=unreal.EOFeedbackIntensity.NORMAL
    )
    presets["Aircraft_LateralBurst"] = preset(
        sound=first_existing(UI + "/FX_Sounds/Whoosh_1_Cue", UI + "/Glitches/Glitch_10_Cue"),
        volume=0.5,
        pitch_jitter=0.08,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Aircraft_Collision"] = preset(
        sound=first_existing(UI + "/Impacts/Impact_1_Low_Cue", UI + "/Impacts/Impact_1_Cue"),
        effect=first_existing(
            NIAGARA + "/Impacts/NS_Impact_Metal",
            BIGFX + "/NiagaraEffectMix4/Effects/NS_SparksSwarm",
        ),
        shake=shake_class("EOShake_AircraftCollision"),
        effect_scale=2.5,
        fov=-4.0,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )
    presets["Aircraft_Scrape"] = preset(
        sound=first_existing(UI + "/Glitches/Glitch_11_Cue"),
        effect=first_existing(
            BIGFX + "/NiagaraEffectMix4/Effects/NS_SparksSwarm",
            NIAGARA + "/Impacts/NS_Impact_Metal",
        ),
        volume=0.45,
        pitch_jitter=0.12,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )

    # ---- Deployment ---------------------------------------------------------
    presets["Deploy_Launch"] = preset(
        sound=first_existing(VOX + "/HumanMaleA/Cues/voice_male_effort_grunt_long_01_Cue"),
        effect=first_existing(
            BIGFX + "/NiagaraEffectMix4/Effects/NS_Jets",
            BIGFX + "/NiagaraEffectMix3/Effects/NS_DustActive",
        ),
        shake=shake_class("EOShake_DeployLaunch"),
        attach_effect=True,
        fov=12.0,
        fov_decay=0.5,
        intensity=unreal.EOFeedbackIntensity.SIGNATURE,
    )
    presets["Deploy_Land"] = preset(
        sound=first_existing(FOOT + "/Concrete/Footstep_Concrete_Boots_Land_4_Cue"),
        effect=first_existing(
            BIGFX + "/NiagaraEffectMix3/Effects/NS_DustActive",
            BIGFX + "/NiagaraEffectMix3/Effects/NS_Dust",
        ),
        shake=shake_class("EOShake_HardLanding"),
        volume=1.2,
        effect_scale=1.8,
        shake_scale=1.3,
        intensity=unreal.EOFeedbackIntensity.SIGNATURE,
    )

    # ---- Ground movement ----------------------------------------------------
    presets["Parkour_Vault"] = preset(
        sound=first_existing(VOX + "/HumanMaleA/Cues/voice_male_effort_grunt_01_Cue"),
        volume=0.7,
        pitch_jitter=0.1,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Parkour_Mantle"] = preset(
        sound=first_existing(VOX + "/HumanMaleA/Cues/voice_male_effort_grunt_02_Cue"),
        volume=0.8,
        pitch_jitter=0.1,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Parkour_Climb"] = preset(
        sound=first_existing(VOX + "/HumanMaleA/Cues/voice_male_effort_grunt_long_02_Cue"),
        volume=0.9,
        pitch_jitter=0.08,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    # Hand or boot meeting the obstacle. Metal, because that is what a greybox
    # city is made of, and it is the one contact the player hears every time.
    presets["Parkour_Contact"] = preset(
        sound=first_existing(FOOT + "/Metal/Footstep_Metal_Boots_Jump_1_Cue"),
        volume=0.8,
        pitch_jitter=0.12,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Parkour_Slide"] = preset(
        sound=first_existing(FOOT + "/Concrete/Footstep_Concrete_Boots_Run_Stop_4_Cue"),
        effect=first_existing(
            BIGFX + "/NiagaraEffectMix3/Effects/NS_DustLow_Active",
            BIGFX + "/NiagaraEffectMix4/Effects/NS_SparksSwarm",
        ),
        attach_effect=True,
        volume=1.0,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    presets["Parkour_SlideEnd"] = preset(
        sound=first_existing(FOOT + "/Concrete/Footstep_Concrete_Boots_Jog_Stop_4_Cue"),
        volume=0.6,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Move_LandLight"] = preset(
        sound=first_existing(FOOT + "/Concrete/Footstep_Concrete_Boots_Land_4_Cue"),
        volume=0.55,
        pitch_jitter=0.1,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Move_LandHard"] = preset(
        sound=first_existing(FOOT + "/Concrete/Footstep_Concrete_Boots_Land_4_Cue"),
        effect=first_existing(BIGFX + "/NiagaraEffectMix3/Effects/NS_Dust_Low"),
        shake=shake_class("EOShake_HardLanding"),
        volume=1.1,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )

    # ---- Takedown -----------------------------------------------------------
    # Commit is the input acknowledgement and nothing more: it fires a frame
    # before the impact, and anything louder would step on it.
    presets["Takedown_Commit"] = preset(
        sound=first_existing(UI + "/Clicks/Click_Low_Cue", UI + "/Clicks/Click_Cue"),
        volume=0.5,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Takedown_Impact"] = preset(
        sound=first_existing(UI + "/Impacts/Impact_2_Low_Cue", UI + "/Impacts/Impact_2_Cue"),
        effect=first_existing(
            "/Game/Vefects/Easy_Impact_Frames/VFX/NS_Impact_Frame_01",
            NIAGARA + "/Impacts/NS_Impact_Concrete",
        ),
        shake=shake_class("EOShake_TakedownImpact"),
        volume=1.1,
        # Local only, and short. Perception mode owns real time manipulation later.
        hit_stop=0.08,
        intensity=unreal.EOFeedbackIntensity.SIGNATURE,
    )

    # ---- Pistol -------------------------------------------------------------
    presets["Pistol_Fire"] = preset(
        sound=first_existing(
            GUN + "/Handgun/Gunshots/handgun_gunshot_01_Cue",
            "/Game/NW_MuzzleFX/Sound/Sfx/SC_Shot_001",
        ),
        effect=first_existing(
            MUZZLE + "/FXS_Pistol_MuzzleFlash",
            MUZZLE + "/FXS_NS_MuzzleFlash_02",
            NIAGARA + "/MuzzleFlashes/NS_MuzzleFlash",
        ),
        shake=shake_class("EOShake_PistolRecoil"),
        pitch_jitter=0.06,
        attach_effect=True,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )
    presets["Pistol_HitHard"] = preset(
        sound=first_existing(LYRA + "/Impacts/Lyra_ImpactConcrete_01", UI + "/Impacts/Impact_1_Cue"),
        effect=first_existing(NIAGARA + "/Impacts/NS_Impact_Concrete"),
        volume=0.8,
        pitch_jitter=0.1,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    presets["Pistol_HitMetal"] = preset(
        sound=first_existing(LYRA + "/Impacts/Lyra_ImpactMetal_01", UI + "/Impacts/Impact_2_Top_End_Cue"),
        effect=first_existing(
            NIAGARA + "/Impacts/NS_Impact_Metal",
            BIGFX + "/NiagaraEffectMix4/Effects/NS_SparksSwarm",
        ),
        volume=0.8,
        pitch_jitter=0.1,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    presets["Pistol_HitCharacter"] = preset(
        sound=first_existing(VOX + "/HumanMaleB/Cues/voice_male_grunt_pain_01_Cue"),
        effect=first_existing(NIAGARA + "/Impacts/NS_Impact_Wood"),
        volume=1.0,
        pitch_jitter=0.12,
        effect_scale=0.7,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )

    # ---- Incoming fire ------------------------------------------------------
    presets["Guard_Fire"] = preset(
        sound=first_existing(GUN + "/Handgun/Gunshots/handgun_gunshot_02_Cue"),
        effect=first_existing(MUZZLE + "/FXS_NS_MuzzleFlash_03", NIAGARA + "/MuzzleFlashes/NS_MuzzleFlash"),
        volume=0.9,
        pitch_jitter=0.08,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    # The whole point of the verb: a near miss should move the player before any
    # health is lost. Lyra's whizz-bys are the only thing in the library for this.
    presets["Guard_NearMiss"] = preset(
        sound=first_existing(
            LYRA + "/WhizBys/Lyra_BulletIn_Close_01",
            LYRA + "/WhizBys/Lyra_BulletIn_Close_02",
        ),
        volume=1.0,
        pitch_jitter=0.15,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )
    presets["Player_Damaged"] = preset(
        sound=first_existing(VOX + "/HumanMaleA/Cues/voice_male_grunt_pain_03_Cue"),
        shake=shake_class("EOShake_PlayerDamage"),
        volume=1.0,
        pitch_jitter=0.1,
        vignette=0.75,
        vignette_decay=0.5,
        directional=True,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )

    # ---- Detection ----------------------------------------------------------
    # One cue per rung, deliberately distinct in register: a rising tone for
    # suspicion, a hard confirm, a searching pulse, a falling all-clear.
    presets["Guard_Suspicious"] = preset(
        sound=first_existing(UI + "/Rings/Reverse_Ring_Cue", UI + "/Clicks/Click_Scoop_Up_Cue"),
        volume=0.7,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    presets["Guard_DetectConfirmed"] = preset(
        sound=first_existing(UI + "/Glitches/Glitch_12_Cue", UI + "/Impacts/Impact_2_Reso_Cue"),
        volume=1.0,
        # Restrained, and only here. Detection does not shake the camera.
        vignette=0.35,
        vignette_decay=0.8,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )
    presets["Guard_Searching"] = preset(
        sound=first_existing(UI + "/Rings/Reverse_Ring_2_Mid_Cue"),
        volume=0.6,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["Guard_LostContact"] = preset(
        sound=first_existing(UI + "/Rings/Reverse_Ring_2_Low_Cue"),
        volume=0.7,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    presets["Guard_Death"] = preset(
        sound=first_existing(VOX + "/HumanMaleC/Cues/voice_male_grunt_pain_death_01_Cue"),
        volume=1.0,
        pitch_jitter=0.1,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )

    # ---- Mission ------------------------------------------------------------
    presets["Objective_Complete"] = preset(
        sound=first_existing(UI + "/Click_Combos/Click_Combo_3_Cue"),
        volume=0.9,
        intensity=unreal.EOFeedbackIntensity.NORMAL,
    )
    presets["Extraction_Call"] = preset(
        sound=first_existing(UI + "/Click_Combos/Click_Combo_5_Cue", UI + "/Rings/Reverse_Ring_2_Cue"),
        volume=1.0,
        intensity=unreal.EOFeedbackIntensity.STRONG,
    )
    presets["Extraction_Board"] = preset(
        sound=first_existing(UI + "/Impacts/Impact_2_Mid_Cue"),
        shake=shake_class("EOShake_ExtractionBoard"),
        volume=1.0,
        fov=5.0,
        intensity=unreal.EOFeedbackIntensity.SIGNATURE,
    )

    # ---- UI -----------------------------------------------------------------
    presets["UI_ActionAvailable"] = preset(
        sound=first_existing(UI + "/Clicks/Click_1_Cue"),
        volume=0.5,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )
    presets["UI_ActionUnavailable"] = preset(
        sound=first_existing(UI + "/Clicks/Click_Pitched_Down_Cue"),
        volume=0.5,
        intensity=unreal.EOFeedbackIntensity.SUBTLE,
    )

    return presets


def main():
    full_path = "{}/{}".format(PACKAGE_PATH, ASSET_NAME)

    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        asset = unreal.EditorAssetLibrary.load_asset(full_path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.EOFeedbackPresetSet)
        asset = tools.create_asset(ASSET_NAME, PACKAGE_PATH, unreal.EOFeedbackPresetSet, factory)

    if asset is None:
        unreal.log_error("Could not create or load {}".format(full_path))
        return

    presets = build_presets()
    asset.set_editor_property("presets", presets)

    unreal.EditorAssetLibrary.save_asset(full_path)

    bound = sum(
        1
        for p in presets.values()
        if p.get_editor_property("sound") is not None
        or p.get_editor_property("effect") is not None
    )

    unreal.log(
        "Feedback presets: {} events, {} with at least one asset bound.".format(
            len(presets), bound
        )
    )

    if _missing:
        unreal.log(
            "Packs not found (those events fall back or stay silent): {}".format(
                ", ".join(sorted(set(m.split("/")[2] for m in _missing if m.count("/") > 2)))
            )
        )

    unreal.log(
        "Set this asset on Project Settings > Executive Ops - Feedback > Preset Set, "
        "or in DefaultGame.ini under [/Script/ExecutiveOps.EOFeedbackSettings]."
    )


main()
