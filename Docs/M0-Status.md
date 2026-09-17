# M0 — Status

**M0 is complete.** The project boots, both pawns are possessable, and the full
aircraft -> operative -> aircraft transition drives the mission state machine correctly.

## Done when

> The project reliably boots into a test map and the player can possess either the
> operative or a placeholder aircraft.

Verified automatically. `Scripts/` builds the content, and an in-engine self-test
exercises the loop:

```bash
UnrealEditor-Cmd.exe ExecutiveOps.uproject /Game/Maps/L_FlightTest -game -nullrhi -unattended -EOSelfTest -EOSelfTestExit
```

19 checks, all passing, process exit code 0:

- boots possessing the aircraft, mission `Inactive`
- possessed pawn implements the aircraft interface
- deployment refused while not hovering; accepted while hovering and slow
- deployment possesses the operative and leaves the mission `OnGround`
- extraction returns control to the aircraft
- reset restores the boot state
- illegal mission transitions are rejected without corrupting state

## Implement list

| Item | State | Where |
|---|---|---|
| UE5 project | Done | `ExecutiveOps.uproject`, `Config/`, `Source/*.Target.cs` |
| Source control | Done | git + `.gitignore` + `.gitattributes` (LFS for `.uasset`/`.umap`) |
| Enhanced Input | Done | `Input/EOInputConfig.*`, forced on in `Config/DefaultEngine.ini` |
| Basic game/state structure | Done | `Core/EOGameModeBase`, `Core/EOGameStateBase`, `Mission/EOMissionSubsystem` |
| Player controller | Done | `Core/EOPlayerController` |
| Third-person character | Done | `Character/EOOperativeCharacter` + `BP_Operative` (UE4 Mannequin) |
| Parkour animation packs | Done | `Content/OpenWorldAnimset`, `Content/FightingAnimsetPro` (1,438 animations) |
| Empty flight map | Done | `Content/Maps/L_FlightTest` — 26 actors, greybox playground |
| Empty mission map | Done | `Content/Maps/L_MissionTest` — 6 actors, floor and lighting |
| Simple debug UI | Done | `UI/EODebugHUD` — canvas-drawn, no UMG asset |
| Restart/reset command | Done | `Debug/EOCheatManager` — `EOReset`, `EORestartLevel` |

## Technical decisions — interfaces established

| Concern | Contract |
|---|---|
| Aircraft control | `IEOAircraftControlInterface` |
| Player deployment | `IEODeployableInterface` |
| Mission start | `UEOMissionSubsystem::StartMission` / `BeginDeployment` |
| Mission complete | `UEOMissionSubsystem::CompleteObjective` / `CompleteMission`, `IEOMissionParticipantInterface` |
| Extraction | `IEOExtractionInterface`, `UEOMissionSubsystem::BeginExtraction` |

## Build

| | Version |
|---|---|
| Unreal Engine | 5.8 |
| MSVC | 14.44.35207 (VS Build Tools 2022 17.14) |
| Windows SDK | 10.0.26100.0 |
| .NET Framework SDK | 4.8.1 (required by `SwarmInterface` via `UnrealEd`) |

Both `ExecutiveOpsEditor` and `ExecutiveOps` compile and link clean.

## What was deliberately not built

Per "do not build generalized frameworks beyond what the next milestones use":

- No flight model — the aircraft moves, badly, on purpose. M1 owns handling.
- No parkour, traversal detection, or animation blueprint. The packs are imported and
  retarget-ready, but nothing consumes them yet. M4.
- No AI, perception, weapons, health, or takedown. M5.
- No objectives, mission definitions, or mission generation. M6.
- No aircraft arrival behaviour behind `IEOExtractionInterface`. M7.
- No UMG, no game-feel tuning, no audio, no VFX. M8.
- No perception mode. M9.

## Known gaps

- **The animation packs are not in source control** (~1.2 GB, which would exceed
  GitHub's free Git LFS quota). Restore them with `Scripts/import_animation_packs.ps1`.
  Until then `BP_Operative` has no skeletal mesh and the operative is invisible.
- **The operative has no animation blueprint.** The mannequin is assigned but will
  T-pose. Locomotion is M4's job; M0 only required the packs be present.
- **Input has not been verified by hand.** The self-test drives the loop through code,
  so it proves the transitions and that bindings register without error — it does not
  prove that `W` moves the aircraft. Worth five minutes in PIE.
- `AEOPlayerController` spawns an aircraft if the level contains none. `L_FlightTest`
  now has a placed one, so this fallback only covers `L_MissionTest`.
