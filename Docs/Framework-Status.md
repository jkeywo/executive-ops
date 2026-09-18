# Framework Status — M0 to M7

The minimal framework is complete. Every basic verb the concept needs is in the
project, and the whole loop runs end to end:

```text
CITY MAP -> FLY -> APPROACH MISSION -> DEPLOY -> PARKOUR / STEALTH
         -> ASSASSINATE / SHOOT / EVADE -> OBJECTIVE -> EXTRACT -> FLY AWAY
```

Per the milestone document, this is the point to **stop adding major features**
and start refining the verbs (M8).

## Verification

```bash
./Scripts/run_selftests.ps1
```

Module-level checks run separately, and individually, through Unreal's own
automation framework - in the editor's Session Frontend, or headless:

```bash
UnrealEditor-Cmd.exe ExecutiveOps.uproject -ExecCmds="Automation RunTests ExecutiveOps+Quit" -unattended -nullrhi
```

These do not play the game: the mission checks build a bare world, exercise the
subsystem through its interface and tear it down, in about a second. See
`Docs/adr/0007`.

Two suites, because the flight sequence and the ground arena live in different
maps. 186 checks total, stable across repeated runs.

| Suite | Map | Checks | Covers |
|---|---|---|---|
| flight | `L_FlightTest` | 59 | boot, flight model, navigation, the approach, deployment, handover |
| ground | `L_MissionTest` | 127 | traversal, slide, the guard encounter, the mission run twice, extraction |

The suites drive the game through its public interfaces and assert on real
outcomes: the aircraft flies its own 268m approach, the operative rides the drop
down, the guard encounter is exercised for all five outcomes, and the mission is
completed twice with a reset in between.

## What each milestone left deliberately unbuilt

| Milestone | Deliberately deferred |
|---|---|
| M1 Flight | Handling is functional, not tuned. Engine audio is a hook with no asset. |
| M2 District | One district, greybox. No streaming or World Partition work. |
| M3 Deployment | One fixed socket, no insertion choice, no cloak, no mission timer. |
| M4 Ground movement | No animation blueprint: clips are played single-node, so transitions pop. No wall running, grapple or dash. |
| M5 Guard | One guard. No alert propagation, no shared knowledge, no cover system. |
| M6 Mission | One fixed arena, one trivial objective, no mission generation. |
| M7 Extraction | Scripted arrival only. No aircraft support, no autonomous navigation. |

## Known gaps

- **No animation blueprint.** Locomotion is selected in C++ and played through
  `PlayAnimation`, so there is no blending and transitions snap. This is the
  single most visible rough edge and belongs in M8.
- **No audio or VFX.** *(M8: superseded. Feedback is now dispatched through
  `UEOFeedbackSubsystem` from data presets - see `Docs/M8-Feedback.md`. Aircraft
  engine audio remains genuinely missing from the library.)*
- **The asset packs are not in source control** (several GB). Restore them with
  `Scripts/import_fab_assets.ps1`.
- **Nothing has been played by a human.** Everything here is verified by the
  self-test suites, which prove the systems do what they claim. They cannot
  tell you whether any of it is enjoyable - which is the only question M8 asks.

## Next

M8 is a game-feel pass over the verbs, one at a time, with no new features.
The milestone document is explicit that if the prototype feels weak, the answer
is timing, control, camera, input assistance and feedback - not content.
