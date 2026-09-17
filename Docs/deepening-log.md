# Deepening log

Running record of the eight-candidate architecture pass on the
`architecture-deepening` branch. Decisions taken without the user present are
marked **[autonomous]** and are the ones worth reviewing first.

Directions were settled by grilling before implementation began; the reasoning
behind each lives in `Docs/adr/`. This file records what actually happened,
including the places where the agreed direction met something the codebase did
not support.

## Baseline

Editor target builds clean (27s incremental). Both self-test suites run from
`Scripts/run_selftests.ps1`, and the suite result is the gate for every commit
below.

## Candidates

### C6 - Load the feedback presets off the hot path  *(done)*

Async warm at `Initialize`, events skip silently until their asset lands, and
event identity moves from `FName` to native gameplay tags.

- **Correction to the review.** The review said GameplayTags "is not enabled in
  this project at all", implying a plugin. It is an engine *module*, not a
  plugin, so adopting it cost one line in `ExecutiveOps.Build.cs` and no
  `.uproject` change. GameplayAbilities is the plugin; that is still not used.
- **[autonomous]** Call sites were left untouched. `FNativeGameplayTag` converts
  implicitly to `FGameplayTag`, so all 33 `EOFeedbackEvents::X` uses compile
  unchanged and the diff stays in the feedback module rather than spreading
  across five gameplay files.
- **[autonomous]** Tag strings are `Feedback.<Category>.<Event>`, derived from the
  existing single-underscore names. The Python generator keeps the readable
  `Pistol_Fire` keys and converts at the point of writing, so the generator still
  diffs against `EOFeedbackEvents.h` one symbol per line.
- **[autonomous]** `unreal.GameplayTag(tag_name=...)` and `set_editor_property`
  both fail - `TagName` is read-only from Python. `import_text` is the only route
  the struct exposes, and is what the generator uses.
- Verified by reading the regenerated asset back: 34 presets, every key a
  `Feedback.*` tag, none left as a bare name.

### C1 - Give the deployment sequence one owner  *(partially done)*

**Done.** `ControlMode` is deleted. `GetControlMode()` now derives the answer from
the possessed pawn, asked through `IEOAircraftControlInterface` and
`IEODeployableInterface` rather than by comparing against cached pointers, so a
pawn this controller never resolved still answers correctly. The mapping context
moved to an `OnPossess` override, so it follows possession itself instead of
being applied by the two request methods - which is what made the stored value go
stale when the game mode possessed the boot pawn first.

Ending a drop is now a single writer, `EndDeploymentDrop()`. It was three, and
`SetStowed(true)` - which is how an abandoned drop actually ends when the player
flies off - cleared neither the flag nor the timer. That worked only because
every caller remembered to cancel first. It now closes by construction.

**[autonomous] Not done: moving the drop timeout into the mission subsystem.**
The grilled decision was that the mission module owns the sequence and both
recovery timers. Implementing that turned out to require the subsystem to hold an
operative reference and to perform the timeout recovery itself - which is
teleporting a pawn to an insertion point and resetting its movement mode. That is
pawn behaviour, and putting it in the mission module would buy timer co-location
at the cost of the subsystem knowing how to recover a character. Judged a worse
trade than the problem it solves, so the timer stays on the character and the
three-way drift it caused is fixed by the single-writer change instead.

Worth revisiting if the operative ever becomes an `IEOMissionParticipantInterface`
implementer - the subsystem already pushes state to participants, and that is the
route by which it could own the sequence without holding pawn pointers.

### C5 - The aircraft's integrator is written twice  *(done)*

`UEOAircraftMovementComponent : UFloatingPawnMovement` now owns velocity and the
move. The sweep / penetration-escape / slide-along-surface block existed twice -
once in `UpdateFlight`, once in `UpdateScriptedArrival` - and is now one method
both paths call.

- The pawn still decides what the craft is trying to do: throttle curves, hover
  blend, attitude, the scripted approach gain. Only the move crossed over, which
  is the split the grilling settled on.
- **[autonomous]** `TickComponent` deliberately does not call `Super`.
  `UFloatingPawnMovement` applies control input to velocity and then moves, which
  would both fight the pawn's flight model and move the craft twice in a frame.
  What this wants from the base class is its velocity plumbing, not its
  integration. The pawn calls `MoveByVelocity` from its own tick so that deciding
  and moving stay in order within the frame.
- **[autonomous]** Impacts come back out of the component as a delegate rather
  than the component calling the pawn, so it does not need to know what feedback
  is.
- `SlideRetentionPerSecond` moved to the component at the same 0.15 default.
  Checked that no override was lost: `m0_setup.py`'s aircraft configuration sets
  only the HUD screen material, so the Blueprint never carried a value for it.

### C2 - Deepen the weapon out of the operative  *(done)*

**Done: damage goes through Unreal's pipeline.** Both shooters call
`UGameplayStatics::ApplyPointDamage`; `UEOHealthComponent` binds its owner's
`OnTakeAnyDamage` instead of exposing a method for shooters to find and call.
Neither shooter looks for a health component to decide whether it can hurt
something any more.

The takedown was the subtle part. It used to bypass damage entirely - `Kill()`
fired `OnDied` but never `OnDamaged`, and that absence was what stopped a silent
kill triggering the guard's "someone shot me" reaction. With one route in, the
distinction is carried by `UEOTakedownDamageType` and the guard ignores damage of
that type. The behaviour is identical; the reason it works is now data on the
event rather than a choice of function.

- **[autonomous]** `Kill()` is deleted rather than kept alongside. Two doors into
  the same state is the shape this whole review keeps flagging, and the takedown
  was its only caller.
- **[autonomous]** `ApplyDamage` stays public, documented as the direct route for
  the reset command and the tests, which need a deterministic poke that does not
  depend on an event being routed. Gameplay does not use it.
- **[autonomous]** `FindComponentByClass` survives in the operative's shot, but
  only to choose between a flesh and a concrete impact preset. That is a
  presentation question, not the route damage takes.

**Done: `UEOWeaponComponent`.** One call, `TryFire(muzzle, aimPoint, bAccurate)`,
owning the cooldown, the spread cone, the `ECC_Pawn` trace and the damage. Both
characters fire through it.

- Aiming stays with the caller, which is the split the grilling settled on and
  the only part that genuinely differed: the operative aims down a camera the
  guard does not have. Impact feedback stays with the caller too, because what an
  impact means depends on what was hit - the operative picks a surface preset,
  the guard places a near-miss whizz.
- **[autonomous]** The guard's `ShotDamage`, `ShotSpread` and `FireInterval` are
  gone, their values carried onto the component in its constructor (18 damage,
  5 degrees, 0.85s, 2600 range). A guard shoots weaker, slower and less
  accurately than the operative; that was worth preserving exactly.
- **[autonomous]** The guard kept a second cooldown of its own, counted down in
  its tick, which would have left two numbers that had to agree. It now asks the
  weapon whether it is ready. What survives is `FirstShotRemaining` - the beat
  between being alerted and the first shot - which is a different idea from the
  rate of fire and was only ever sharing the variable.
- Two automation tests came with it, which is the point of the candidate: the
  cooldown refuses a second shot inside the interval, and the spread cone stays
  inside its own bound while aiming removes it. The second is the property the
  flaky gun check actually hinged on, and it was previously only observable by
  taking real shots at a real guard.
- **[autonomous]** I first wrote a third test asserting that a shot damages what
  it hits, and it failed - not in the weapon, in the fixture. A hand-built
  `UWorld` never ran `BeginPlay`, so the target's health sat at zero, and the
  actor would not take the spawn transform once its root was swapped. Rather
  than keep fighting the fixture I removed it: a check that needs real collision
  and a real `BeginPlay` is a world check, and the grilling already decided those
  belong in an `AFunctionalTest` in a map rather than in an automation test. The
  damage path is still covered end to end by the ground suite's guard encounter.

### C8 - Two input settings that cannot take effect  *(partially done)*

**Done: the settings are real.** `bInvertMouseY` and `bInvertStickY` moved to
`UEOInputSettings` (a `UDeveloperSettings`) and are read where a look value is
consumed, in all four handlers, rather than baked into a `UInputModifierNegate`
at context-build time. Changing one now applies immediately. The mappings no
longer carry the inversion at all.

- **[autonomous] Deviation from the grilled decision.** The grilling chose
  Enhanced Input's own user settings, which persist per user. That needs a
  settings subclass, `bEnableUserSettings`, and save-game plumbing, and it is a
  player-facing options feature rather than the defect. `UDeveloperSettings`
  matches the pattern already in the project for feedback and makes the setting
  work today. Recorded as `Docs/adr/0008` so the intended destination is not
  lost.

**Not done: moving the mappings to IMC assets**, and `UEOInputConfig` becoming a
`UDataAsset`. That is asset generation work on the scale of the existing Python
build scripts, and with the inversion no longer baked in, the config being
rebuilt each run is no longer a correctness problem - only a
not-authorable-in-the-editor one. It is the remaining half of this candidate.

### C4 - Make the checks independent of the run  *(partially done)*

**Done: the first checks that do not require playing the game.**
`Source/ExecutiveOps/Tests/EOMissionSubsystemTest.cpp` puts three checks on the
engine's automation seam: the legal route through the loop, an illegal
transition being refused without touching state, and reset returning to inactive
from mid-mission. They run from the Session Frontend or headless, individually,
in about a second, and a failure names the module rather than the phase the run
happened to die in.

The mission state machine was chosen first because it needs nothing but a world:
no map, no actors, no pawns. `FScopedTestWorld` builds and tears one down, which
is the pattern the rest of the module-level checks can follow.

- **[autonomous]** The tests sit in the existing module under
  `#if WITH_AUTOMATION_TESTS` rather than in a separate one. The grilling chose a
  separate module excluded from the Game target, and that is still right for
  `UEOSelfTest` and `UEOCheatManager` - but it cannot be done by moving files
  alone: `AEOPlayerController` constructs the self-test and sets `CheatClass`, so
  a runtime module would depend on a developer module, which is backwards.
  Inverting that needs a registration hook, and is its own job. `WITH_AUTOMATION_TESTS`
  is 0 in shipping, so the new tests do not ship either way.

**Not done:** converting the 143 existing checks, converting the two suites to
`AFunctionalTest`, and moving the self-test and cheat manager out of the shipping
module.

## What was not implemented

Four candidates are wholly or largely untouched. Each has a settled direction
recorded in `Docs/adr/`, so the decision work is not lost - only the execution
remains.

**C3 - one HUD state, two adapters, on UMG.** The largest single job here: 1,184
lines of Canvas drawing across three `AHUD` classes, plus widget assets, plus
replacing `EOHudScreenComponent`'s render-target trick with a
`UWidgetComponent`. Nothing was started, deliberately - a half-migrated HUD is
worse than either end state, and the gather seam is only worth building once the
widgets that consume it exist.

**C4 - the rest.** Converting the 143 existing checks, turning the two suites
into `AFunctionalTest`s, and moving `UEOSelfTest` and `UEOCheatManager` out of the
shipping module. The last of those is blocked on a dependency inversion, noted
above.

**C7 - the guard's AI.** Untouched. Perception, StateTree, AIController and
navmesh together are a rewrite of the one system whose current shape is
deliberate and documented, inside a milestone defined as adding no features. It
is the largest scope expansion of the eight and the one I would most want the
author awake for. `Docs/adr/0005` records the decision to do it and why that
contradicts the milestone doc.

## Suggested order for the rest

C4's conversion, then C3, then C7. C4's conversion makes every later change safer
to verify, and the two test files added here show the pattern to follow. C3 and
C7 are both asset-authoring jobs and are better done with the editor open.
