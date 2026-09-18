# Animation

How the operative is animated, and what is left to do by hand.

---

## What changed, and why

The operative used to be animated by C++ picking a clip and calling
`PlayAnimation` on the mesh — single-node playback. That is not an animation
system, and it produced exactly the faults it was always going to produce:

| Symptom | Cause under single-node playback |
|---|---|
| Run and fall reset part-way through | `PlayAnimation(anim, bLooping=false)` on a clip that needs to loop |
| No landing animation | There was no landing state to be in |
| Run starts with a slow lean forward | The jog clip's own start-up frames replayed from zero on every re-trigger |
| Legs desync and blink on direction change | Clips were swapped outright, with no blending and no phase matching |
| Character moved artificially | Root motion was deliberately disabled and the root bone locked, so the capsule moved and the feet followed |

All of it is now driven by an Animation Blueprint: `/Game/Animation/ABP_Operative`.

---

## Where the graph came from

It is the **Dynamic Locomotion** pack's own locomotion graph, duplicated into
the project and repointed at project-owned assets by
`Scripts/m9_prepare_animation.py`.

That pack was already in the Fab library, and it ships exactly the state machine
this project needs — starts, a walk/jog/run blend space, foot-phased left and
right stops, jump start, fall loop, and both stationary and moving landings —
with **sync markers already authored on every clip** (`Step1`, `Step2`,
`Step3`). Those markers are what stop the legs desyncing: the engine matches
playback position by marker rather than by proportion, so a run cycle blending
into a walk cycle keeps its footfalls aligned.

Unreal's Python API cannot *create* AnimGraph nodes, but it can read and write
the properties of nodes that already exist. So the script duplicates the pack's
working graph rather than trying to author one, then redirects its blend space
player and sequence players at the project's own copies.

### Rebuilding it

```
UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript -script="C:/Coding/executive-ops/Scripts/m9_prepare_animation.py"
```

Idempotent: it detaches the old graph from `BP_Operative`, deletes the previous
duplicates in dependency order, and rebuilds. Add `-rootmotion` to repoint every
clip at the pack's root-motion twins — see below.

The two UE4 mannequin skeletons — ours from Open World Animset, the pack's from
Dynamic Locomotion — are marked **compatible** in both directions rather than
retargeted. They are the same rig; they are only separate assets because they
arrived in separate packs.

---

## Speeds and the blend space

`/Game/Animation/BS_Locomotion` keeps the pack's authored sample positions: X is
lean (-1 to 1), Y is speed in cm/s, with clips at **180 (walk), 500 (jog) and
950 (run)**.

Those are the speeds the clips were actually captured at, and that is what stops
the feet sliding — the character's speeds should be matched to the blend space,
not the other way round. Currently `WalkSpeed` is 500 (lands exactly on the jog
sample) and `SprintSpeed` is 850 (blends jog toward run).

Nothing currently drives the character below 500, so the walk clip is never
reached. A walk modifier or analogue stick input would open up the lower half of
the space.

---

## Root motion

Not on by default. `-rootmotion` switches the blend space samples and every
sequence player to the pack's `_RM` clips and enables root motion on them.

The reason it is opt-in rather than the default: **root motion replaces velocity
while it is active**, and three systems in this project move the capsule
themselves — the slide writes velocity every tick, `EOTraversalComponent` drives
the capsule along an authored arc, and deployment throws the operative clear
under its own launch velocity. With `RootMotionFromEverything` those all stop
working, because the animation's zero root motion wins.

Doing it properly means switching root motion mode per state from C++
(`UAnimInstance::SetRootMotionMode`), leaving it on for free locomotion and off
while sliding, traversing or deploying. That is a contained piece of work, but it
is a movement-model change rather than an animation fix, so it is not bundled in
here.

The more common shipped-game arrangement — and what is in place now — is
in-place locomotion driven by the movement component, with root motion reserved
for authored one-shots where the clip must dictate the exact distance: vaults,
takedowns, landings.

---

## What still needs the editor

**The graph has no Slot node**, which means montages cannot play through it. That
is the one thing Python cannot do, because it requires creating a node.

Until it exists, the one-shots are silent — they are gated off in code
(`AEOOperativeCharacter::UsesDirectAnimationPlayback`) because `PlayAnimation`
would switch the mesh back to single-node mode and throw the whole graph away.
So **pistol fire, takedown, death, the vault/mantle/climb clips and the aim
strafe set currently do not play.** Locomotion is correct; those are not.

### Adding the slot

1. Open `/Game/Animation/ABP_Operative`.
2. In the **AnimGraph**, find the wire running from the `Idle/Movement` state
   machine into **Output Pose**, and delete it.
3. Right-click in the graph → search for **`Slot 'DefaultSlot'`** → place it.
4. Wire the state machine's **Pose** output into the slot's **Source** input, and
   the slot's output into **Output Pose**.
5. Compile and save.

That alone restores nothing on its own — it is the socket the next step plugs
into. Tell me once it is in and I will convert the one-shots to montages and
re-enable them from C++, which is all automatable from there.

### Optional, same visit

- **Upper-body aiming.** A `Layered blend per bone` node from `spine_01` lets the
  aim pose play over the legs, instead of replacing the whole body as the old
  code did.
- **Inertialization.** Setting the state machine's transitions to use
  inertialization rather than a standard blend removes the last of the pops on
  fast direction changes.

---

## Driving the graph from C++

The pack's graph computes its own variables (`Speed`, `IsInAir?`, `IsMoving`,
`IsAccelerating`, `LateralSpeed`, `StopL`, `IsLandingWhileMobile`) from the pawn
in its event graph, so no C++ anim instance is needed yet.

If the graph later needs game state it cannot read off the movement component —
sprinting, aiming, sliding, traversing — the idiomatic route is a C++
`UAnimInstance` subclass computing them in a thread-safe update, with the graph
reparented to it. That keeps the casts out of the event graph and the state out
of the graph's head.

---

## Fallback

`UsesDirectAnimationPlayback()` returns true only when the mesh is in
single-node mode, so a project whose animation assets have not been prepared —
no packs imported, no Animation Blueprint — still animates through the old
direct path. The guard still uses it; only the operative has been moved over.
