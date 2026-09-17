# M0 — Status

Against the milestone document's **Implement** list:

| Item | State | Where |
|---|---|---|
| UE5 project | Done | `ExecutiveOps.uproject`, `Config/`, `Source/*.Target.cs` |
| Source control | Done | git + `.gitignore` + `.gitattributes` (LFS for `.uasset`/`.umap`/art) |
| Enhanced Input | Done | `Input/EOInputConfig.*`, forced on in `Config/DefaultEngine.ini` |
| Basic game/state structure | Done | `Core/EOGameModeBase`, `Core/EOGameStateBase`, `Mission/EOMissionSubsystem` |
| Player controller | Done | `Core/EOPlayerController` — possession + input context swap |
| Third-person character | Code done | `Character/EOOperativeCharacter` — mesh/anim BP assigned in editor |
| Parkour animation packs | Editor step | `Docs/M0-Editor-Checklist.md` §4 |
| Empty flight map | Editor step | `Docs/M0-Editor-Checklist.md` §1 |
| Empty mission map | Editor step | `Docs/M0-Editor-Checklist.md` §1 |
| Simple debug UI | Done | `UI/EODebugHUD` — canvas-drawn, no UMG asset |
| Restart/reset command | Done | `Debug/EOCheatManager` — `EOReset`, `EORestartLevel` |

Against **Technical decisions** — interfaces established for:

| Concern | Contract |
|---|---|
| Aircraft control | `IEOAircraftControlInterface` |
| Player deployment | `IEODeployableInterface` |
| Mission start | `UEOMissionSubsystem::StartMission` / `BeginDeployment` |
| Mission complete | `UEOMissionSubsystem::CompleteObjective` / `CompleteMission`, `IEOMissionParticipantInterface` |
| Extraction | `IEOExtractionInterface`, `UEOMissionSubsystem::BeginExtraction` |

## What was deliberately not built

Per "do not build generalized frameworks beyond what the next milestones use":

- No flight model — the aircraft moves, badly, on purpose. M1 owns handling.
- No parkour, traversal detection, or animation state machine. M4.
- No AI, perception, weapons, health, or takedown. M5.
- No objectives, mission definitions, or mission generation. M6.
- No aircraft arrival behaviour behind `IEOExtractionInterface`. M7.
- No UMG, no game-feel tuning, no audio, no VFX. M8.
- No perception mode. M9.

## Build status

Both targets compile and link clean (Win64, Development) - 11 source files, no errors,
no warnings:

| Target | Output |
|---|---|
| `ExecutiveOpsEditor` | `UnrealEditor-ExecutiveOps.dll` |
| `ExecutiveOps` | `Binaries/Win64/ExecutiveOps.exe` |

The game target building confirms nothing in M0 is accidentally editor-only.

Toolchain this was built against:

| | Version |
|---|---|
| Unreal Engine | 5.8 |
| MSVC | 14.44.35207 (VS Build Tools 2022 17.14) |
| Windows SDK | 10.0.26100.0 |
| .NET Framework SDK | 4.8.1 (required by `SwarmInterface` via `UnrealEd`) |

## Known gaps

- The two maps and the Blueprint subclasses do not exist yet - see the editor checklist.
  Until `L_FlightTest` exists, the editor will warn that the default map is missing.
- Nothing has been run in PIE yet. The code compiles; the eight-step verification in
  `M0-Editor-Checklist.md` section 5 has not been executed.
- `AEOPlayerController` spawns an aircraft at the player start if the level contains none, so
  an empty map still boots into something controllable. Remove that fallback once the maps
  have placed pawns.
