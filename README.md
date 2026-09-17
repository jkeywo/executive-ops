# Executive Ops

Cyberpunk operative prototype — Unreal Engine 5.8.

Design documents live in [`GDD/`](GDD/). Development follows
[`GDD/minimal-framework-milestones.md`](GDD/minimal-framework-milestones.md).

**Current milestone: M0 — Project Skeleton.**

---

## First-time setup

1. Install Unreal Engine 5.8 via the Epic Games Launcher.
2. `git lfs install` (binary assets are tracked with LFS — see `.gitattributes`).
3. Right-click `ExecutiveOps.uproject` → **Generate Visual Studio project files**.
4. Build the `ExecutiveOpsEditor` target (Development Editor, Win64).
5. Open `ExecutiveOps.uproject`.

See [`Docs/M0-Editor-Checklist.md`](Docs/M0-Editor-Checklist.md) for the steps that must be
done inside the editor (maps, character mesh, animation import).

## Controls

| | Aircraft | Operative |
|---|---|---|
| Move | `WASD` | `WASD` |
| Vertical | `Space` / `Left Ctrl` | — |
| Yaw | `Q` / `E` | — |
| Look | Mouse | Mouse |
| Hover hold | `Left Shift` | — |
| Sprint | — | `Left Shift` |
| Jump | — | `Space` |
| Deploy | `F` (while hovering) | — |

Bindings are declared in C++ in [`EOInputConfig.cpp`](Source/ExecutiveOps/Input/EOInputConfig.cpp)
rather than as `.uasset` files, so a fresh clone is playable with no content to import.

## Debug console commands

Open the console with `` ` ``:

| Command | Effect |
|---|---|
| `EOReset` | Return both pawns to their start transforms, reset mission state |
| `EORestartLevel` | Reload the current level |
| `EOPossessAircraft` | Take control of the aircraft |
| `EOPossessOperative` | Take control of the operative |
| `EODeploy` | Force the aircraft → operative transition |
| `EOExtract` | Force the operative → aircraft transition |
| `EOMissionState` | Log the current mission state |
| `EOToggleDebugHUD` | Show/hide the debug readout |

## Architecture

```
Source/ExecutiveOps/
  Interfaces/   The M0 technical decisions — contracts later milestones implement behind
  Mission/      UEOMissionSubsystem: the single linear mission state machine
  Core/         Game mode, game state, player controller (possession + input context swap)
  Aircraft/     Placeholder VTOL  (real handling is M1)
  Character/    Operative         (parkour is M4, combat is M5)
  Input/        Enhanced Input actions and mapping contexts, declared in C++
  UI/           Canvas debug HUD
  Debug/        Cheat manager
```

The five interfaces named in M0's technical decisions:

| Concern | Contract | Implemented in M0 by |
|---|---|---|
| Aircraft control | `IEOAircraftControlInterface` | `AEOAircraftPawn` |
| Player deployment | `IEODeployableInterface` | `AEOOperativeCharacter` |
| Mission start / complete | `UEOMissionSubsystem` + `IEOMissionParticipantInterface` | — |
| Extraction | `IEOExtractionInterface` | nothing yet (M7) |

Mission state is owned by a `UWorldSubsystem`, not the game mode, so it is reachable from
anywhere and survives the eventual flight-map ↔ mission-map transition.
