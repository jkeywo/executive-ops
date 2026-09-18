# Wiring the guard's StateTree

Everything the tree needs exists in C++ and compiles. What is missing is the
graph, which is an asset, and an asset is authored rather than generated — the
compiler that turns editor data into a runnable tree lives in
`StateTreeEditorModule/Private` with no scripting hook, so this is the one part
of `Docs/adr/0005` that has to be done by hand.

Until a tree is assigned the guard behaves exactly as it always has: its C++
state machine still runs, and every encounter check passes. Assigning a tree is
the whole switch, and clearing it is the whole revert.

## What already exists

| Piece | Where |
|---|---|
| Sight, target tracking, the operative cache | `Combat/EOGuardAIController.h` |
| `UStateTreeAIComponent` named `Brain`, and the `BrainTree` slot | `Combat/EOGuardAIController.h` |
| Tasks and conditions | `Combat/EOGuardStateTreeNodes.h` |
| The verbs the tasks call | `Combat/EOGuardCharacter.h`, under "Verbs the StateTree drives" |

Tasks set the pawn's `EEOGuardState` on entry. That enum stays the published
answer to "what is this guard doing" — the HUD reads it, the encounter checks
assert on it, and a takedown is refused based on it — so the tree can drive
behaviour without becoming the only place that knows what is happening.

## Steps

1. Content Browser → **Artificial Intelligence → State Tree**. Pick schema
   **StateTree AI Component Schema**. Save it as `/Game/AI/ST_Guard`.

2. In the tree's **Context** section, the schema provides the AI controller and
   the pawn. Every node below has a `Guard` binding: bind it to the context pawn,
   cast to `EOGuardCharacter`.

3. Build four sibling states under the root, each with one task:

   | State | Task | Enter conditions |
   |---|---|---|
   | `Patrol` | EO Guard Patrol | none |
   | `Suspicious` | EO Guard Suspicious | EO Guard Detection At Least, Threshold `0.01` |
   | `Engage` | EO Guard Engage | EO Guard Detection At Least, Threshold `1.0` |
   | `Search` | EO Guard Search | EO Guard Lost Contact, Seconds `3.0` |

   Order matters: the first state whose conditions pass is the one selected, so
   `Engage` must sit above `Suspicious`, and `Search` above `Patrol`.

4. Transitions:

   - `Engage` → on **State Completed**, go to `Search`.
   - `Search` → on **State Completed**, go to `Patrol`. The search task succeeds
     when it runs out, which is why it needs no timer condition of its own.
   - Everything else re-evaluates from the root each tick, so `Patrol` and
     `Suspicious` need no explicit transitions — a rising detection alpha will
     select a higher state on its own.

5. Assign the asset: open `BP_Guard`, select the AI controller class
   `EOGuardAIController`, and set **Brain Tree** to `ST_Guard`. If the Blueprint
   does not expose it, set `AIControllerClass` on `BP_Guard` to a Blueprint
   subclass of `EOGuardAIController` that carries the reference.

6. Verify with `./Scripts/run_selftests.ps1`. The ground suite exercises all five
   guard outcomes — patrol, stealth kill, detection, being shot at, losing
   contact — so it is a real check on the wiring rather than a smoke test. If the
   guard stands still, look for `could not path` in the log first: that is a
   missing navmesh, not a broken tree.

## What to expect

The five states map one for one onto what the C++ machine did, so a correctly
wired tree should produce the same 127/127. If a state never activates, the usual
cause is condition ordering in step 3 rather than the conditions themselves.

`Dead` has deliberately no state. Death stops the guard ticking at all, which is
handled on the pawn and would only be noise in the tree.
