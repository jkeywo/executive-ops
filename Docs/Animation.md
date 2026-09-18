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
UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript -script="C:/Coding/executive-ops/Scripts/m9_build_montages.py"
```

The second rebuilds the action montages and needs the graph's slot to exist, so
run it after the first.

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

## One-shots: montages through the slot

Locomotion is a state machine; actions are montages. The graph's
`Slot 'DefaultSlot'` node sits between the state machine and the output pose,
which is what lets an action blend in over the legs rather than replacing the
whole body the way `PlayAnimation` did.

`Scripts/m9_build_montages.py` builds one montage per action from its source
clip and assigns them to `BP_Operative`:

| Montage | Source clip | Blend in / out |
|---|---|---|
| `AM_Pistol_Fire` | `Pistol_shoot_01` | 0.05 / 0.15 |
| `AM_Takedown` | `Anim_TA_ANG_hit_fist` | 0.10 / 0.25 |
| `AM_Death` | `death_aim_chest_01` | 0.15 / 0.30 |
| `AM_Vault` | `Vault_jog` | 0.08 / 0.20 |
| `AM_Mantle` | `High_Ledge_Up_Crouch` | 0.10 / 0.25 |
| `AM_Climb` | `Climb_scrambling_path` | 0.10 / 0.25 |

Blend times are short on purpose: these are reactions, not performances, and a
long blend on a pistol shot reads as the operative thinking about it.

The script refuses to run if the graph has no Slot node, because a montage
played into a slot that is not sampled reports success and silently does
nothing.

`AEOOperativeCharacter::PlayActionAnimation(Montage, Fallback)` is the single
rule for all of them: montage through the graph when there is a graph, direct
clip when there is not. `EOTraversalComponent` calls the same function rather
than keeping a second copy of it.

### Still to do

- **Upper-body aiming.** The aim strafe set is still unused. It wants a
  `Layered blend per bone` node from `spine_01`, so the aim pose plays over the
  legs rather than taking the whole body the way the old code did.
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
