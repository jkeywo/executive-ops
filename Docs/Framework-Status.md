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
./Scripts/run_tests.ps1
```

Everything runs through Unreal's own automation framework - in the editor's
Session Frontend, or headless as above. Two kinds of test; see `Docs/adr/0007`.

**`ExecutiveOps.*`** - nine world-free checks on the mission subsystem, the
weapon and the HUD view model: build the object, exercise it through its
interface, tear it down, in about a second.

**`Project.Functional Tests.*`** - `AFunctionalTest` actors placed in the maps
by `Scripts/place_functional_tests.py`, each runnable on its own. 191 checks,
stable across repeated runs.

| Test | Map | Checks | Covers |
|---|---|---|---|
| FlightModel | `L_FlightTest` | 27 | boot state, the deployment gate, illegal transitions, the flight model, the cockpit panel |
| Approach | `L_FlightTest` | 33 | site selection, the steered approach, the drop, the aircraft held, the handover back |
| Traversal | `L_MissionTest` | 48 | vault, mantle, climb, the negatives, the slide, the mesh staying on the capsule |
| GuardEncounter | `L_MissionTest` | 35 | the five outcomes: takedown, being seen, being shot, shooting, breaking contact |
| MissionLoop | `L_MissionTest` | 48 | the mission run twice with a reset between, and the re-arm after |

They drive the game through its public interfaces and assert on real outcomes:
the aircraft flies its own 268m approach, the operative rides the drop down,
the guard encounter is exercised for all five outcomes, and the mission is
completed twice with a reset in between. The tests in one map share its world
and run in turn, so each ends with the world as it found it.

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
  functional tests, which prove the systems do what they claim. They cannot
  tell you whether any of it is enjoyable - which is the only question M8 asks.

## Next

M8 is a game-feel pass over the verbs, one at a time, with no new features.
The milestone document is explicit that if the prototype feels weak, the answer
is timing, control, camera, input assistance and feedback - not content.
