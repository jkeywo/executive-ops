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

### C2 - Deepen the weapon out of the operative  *(partially done)*

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

**Not done: extracting UEOWeaponComponent.** See the end of this log.

