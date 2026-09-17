# M8 — Feedback Architecture

How game feel is wired in Executive Ops, and how to change it.

Implements the shared feedback architecture from
[`GDD/executive-ops-m8-polish-work-brief.md`](../GDD/executive-ops-m8-polish-work-brief.md) §4.

---

## The shape of it

Gameplay code never spawns a sound, a particle or a camera shake. It names an
event:

```cpp
if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
{
    Feedback->Play(EOFeedbackEvents::Pistol_Fire, Context);
}
```

Everything else is data. A preset asset decides what `Pistol_Fire` means, the
subsystem applies the accessibility settings, and the camera and HUD read back
whatever transient screen state the event left behind.

Three consequences worth knowing:

1. **A missing asset pack is silent, not broken.** Every asset reference in a
   preset is soft. A clone with no packs compiles, launches and plays.
2. **Retuning a verb is a data edit**, not a code change. The only reason to
   touch C++ is to add a new event or a new *kind* of feedback.
3. **Accessibility is central, not per-verb.** Shake, screen effects and haptics
   scale in one place, so nothing can quietly opt out of them.

| File | Role |
|---|---|
| `Feedback/EOFeedbackEvents.h` | Every event the game fires, as a native gameplay tag |
| `Feedback/EOFeedbackTypes.h` | `FEOFeedbackPreset` — the layers one event may use |
| `Feedback/EOFeedbackPresetSet.h` | The data asset: event tag → preset |
| `Feedback/EOFeedbackSubsystem.h` | Dispatch, scaling, transient screen state |
| `Feedback/EOFeedbackSettings.h` | Preset set reference + accessibility scalars |
| `Feedback/EOCameraShakes.h` | Seven tuned impulse shakes |

---

## Setup

```powershell
./Scripts/import_fab_assets.ps1 -List   # what is present, available, missing
./Scripts/import_fab_assets.ps1         # copy the packs into Content/
```

Then build the presets, with the editor **closed**:

```
UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript -script="Scripts/m8_build_feedback_presets.py"
```

This writes `/Game/Feedback/DA_EOFeedbackPresets`, binding each event to whichever
assets it found. Re-run it after importing more packs; it fills gaps and leaves
existing bindings alone. `Config/DefaultGame.ini` already points the settings at
that path.

---

## Events

Named in `EOFeedbackEvents.h`, fired from the classes listed here.

| Event | Fired by | When |
|---|---|---|
| `Aircraft_Accelerate` | `EOAircraftPawn` | Speed rises faster than 75% of max acceleration |
| `Aircraft_BrakeHard` | `EOAircraftPawn` | Speed falls faster than 90% of braking, from above 20% of max |
| `Aircraft_LateralBurst` | `EOAircraftPawn` | Strafe input starts from neutral |
| `Aircraft_Collision` | `EOAircraftPawn` | Swept hit, closing speed into the surface ≥ 35% of threshold |
| `Aircraft_Scrape` | `EOAircraftPawn` | Glancing contact below that, rate-limited to 4/sec |
| `Deploy_Launch` | `EOOperativeCharacter` | Thrown clear of the aircraft |
| `Deploy_Land` | `EOOperativeCharacter` | The drop's own landing |
| `Parkour_Vault` / `_Mantle` / `_Climb` | `EOTraversalComponent` | Traversal accepted — the commitment frame |
| `Parkour_Contact` | `EOTraversalComponent` | At the obstacle apex, not at the character |
| `Parkour_Slide` / `_SlideEnd` | `EOOperativeCharacter` | Slide entry and exit |
| `Move_LandLight` / `_LandHard` | `EOOperativeCharacter` | Landing, split on impact speed vs `HardLandingSpeed` |
| `Takedown_Commit` | `EOOperativeCharacter` | Input accepted, before the kill resolves |
| `Takedown_Impact` | `EOOperativeCharacter` | Contact. Carries the local hit-stop |
| `Pistol_Fire` | `EOOperativeCharacter` | Shot fired |
| `Pistol_HitHard` / `_HitMetal` / `_HitCharacter` | `EOOperativeCharacter` | Chosen by physical material, or by whether the target has health |
| `Guard_Fire` | `EOGuardCharacter` | Guard shoots |
| `Guard_NearMiss` | `EOGuardCharacter` | Miss passing within `NearMissRadius`, placed at the closest point of the shot line |
| `Player_Damaged` | `EOOperativeCharacter` | Damage taken. Drives the vignette and the direction indicator |
| `Guard_Suspicious` / `_DetectConfirmed` / `_Searching` / `_LostContact` / `_Death` | `EOGuardCharacter` | Each rung of the alert ladder |
| `Objective_Complete`, `Extraction_Call`, `Extraction_Board` | `EOMissionSubsystem` | Mission state transitions |
| `UI_ActionAvailable` / `_Unavailable` | *(unused)* | Reserved for the prompt layer |

---

## What a preset can do

| Layer | Fields | Notes |
|---|---|---|
| Audio | `Sound`, `VolumeMultiplier`, `PitchJitter` | Attaches to `Context.AttachTo` when set, otherwise plays at a world point |
| Particles | `Effect`, `EffectScale`, `bAttachEffect` | Niagara |
| Camera | `CameraShake`, `CameraShakeScale` | Scaled by the accessibility setting |
| FOV | `FOVImpulse`, `FOVImpulseDecay` | Positive widens, negative compresses. Read by the aircraft camera |
| Screen | `VignetteIntensity`, `VignetteDecay`, `bDirectionalIndicator` | Drawn by `EOPlayerHUD::DrawScreenFeedback` |
| Haptics | `ForceFeedback` | Skipped entirely when haptics are off |
| Decal | `Decal`, `DecalSize`, `DecalLifetime` | Lifetime-limited: no persistent world damage |
| Timing | `HitStopSeconds`, `HitStopDilation` | **Local** to `Context.Target`, never global |

`Intensity` (Subtle / Normal / Strong / Signature) is editorial rather than
functional — it makes it obvious when a verb has collected three Strong effects
that are fighting each other.

FOV impulses replace rather than accumulate, so two overlapping events cannot
stack into a fisheye.

---

## Accessibility

Project Settings → **Executive Ops - Feedback**, or
`[/Script/ExecutiveOps.EOFeedbackSettings]` in `DefaultGame.ini`.

| Setting | Values | Effect |
|---|---|---|
| `CameraShake` | Off / Reduced / Full | ×0, ×0.5, ×1 on every shake |
| `ScreenEffects` | Off / Reduced / Full | ×0, ×0.5, ×1 on FOV impulse, vignette, direction indicator |
| `bHapticsEnabled` | bool | Force feedback on/off |
| `bMotionBlurEnabled` | bool | Off by default, per the brief's §14 |
| `FeedbackVolume` | 0–2 | Trim on feedback one-shots only |

---

## Camera shakes

`UEOImpulseShakePattern` is a damped, directional kick rather than a noise field.
`Directionality` at 1 gives a one-way shove that eases back (recoil, landing); at
0 it oscillates either side of centre (the collision rattle). The envelope is
squared, so nearly all of the movement is in the first third — which is what
makes a kick read as an impact rather than a wobble.

Only `EOShake_AircraftCollision` uses a low directionality. The brief bans
continuous random screenshake, and reserves rattle for large physical impacts.

| Class | Used by | Duration |
|---|---|---|
| `EOShake_PistolRecoil` | `Pistol_Fire` | 0.16s |
| `EOShake_TakedownImpact` | `Takedown_Impact` | 0.22s |
| `EOShake_HardLanding` | `Move_LandHard`, `Deploy_Land` | 0.30s |
| `EOShake_DeployLaunch` | `Deploy_Launch` | 0.45s, +6° FOV |
| `EOShake_AircraftCollision` | `Aircraft_Collision` | 0.55s |
| `EOShake_PlayerDamage` | `Player_Damaged` | 0.18s |
| `EOShake_ExtractionBoard` | `Extraction_Board` | 0.35s |

---

## Screen feedback

Drawn on the canvas by `EOPlayerHUD::DrawScreenFeedback`, last, over the frame.

- **Vignette** — four edge bands fading inward, not a full-screen overlay. The
  centre of the screen stays clear, which is the whole point: a damaged player
  needs to see where to run.
- **Direction indicator** — a threat arc plus a diamond at the screen-space
  bearing of whatever dealt the damage. Reuses the arc shape the bearing slot
  already uses, so there is one vocabulary to learn rather than two.

Both fade on their own. Neither uses chromatic aberration, blur or a white flash.

---

## Known gaps

- **Aircraft audio.** No engine loops, thrust layers or wind in the library, so
  `Aircraft_Accelerate` and `_BrakeHard` currently carry FOV and camera only. See
  §H of the asset sourcing report — this is the one purchase still worth making.
- **Guard barks are non-verbal.** Human Vocalizations covers pain, death and
  effort across seven actors, but there is no "enemy spotted" vocabulary. The
  detection ladder currently reads through UI cues instead.
- **Physical materials.** `Pistol_HitMetal` needs surfaces that say they are
  metal. The greybox has no physical materials yet, so everything resolves to
  `Pistol_HitHard`.
- **Deployment anticipation.** `Deploy_Ready` is defined but never fired; the
  launch has no anticipation beat yet.
