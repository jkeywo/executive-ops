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

Idempotent, and **non-destructive**: an existing `ABP_Operative` is kept and only
repointed, because the graph is edited by hand — the Slot node, layered blends,
retuned transitions. Pass `-rebuild` to replace it from the pack and lose those.
`-rootmotion` repoints every clip at the pack's root-motion twins — see below.

### Two skeletons, one rig

Ours comes from Open World Animset, the pack's from Dynamic Locomotion. They are
the same UE4 mannequin; they are separate assets only because they arrived in
separate packs. Two things bridge that:

- The skeletons are marked **compatible** in both directions, which is what lets
  the pack's clips play on the operative's mesh without retargeting.
- The **Animation Blueprint itself is retargeted** to the operative's skeleton.
  A skeletal mesh component will not run a graph built for a different skeleton;
  it falls back to the reference pose, which looks exactly like "animation is
  not playing at all". Compatible skeletons cover the clips *inside* the graph —
  the graph itself has to target the mesh's own skeleton.

`add_compatible_skeleton` does not mark the package dirty, so the script saves
those two assets with `only_if_is_dirty=False`. Without that the call appears to
work, reads back correctly in the same session, and is silently gone by the next
one.

---

## Speeds and the blend spaces

The pack's graph plays five blend spaces, one per state, and the script copies
all five: `BS_Locomotion` (the walk/jog/run cycle), `BS_StartMoving`,
`BS_StopMovingL`, `BS_StopMovingR` and `BS_MobileLanding`. The last four are
spreads of one-shot clips across speed, and they have to stay separate assets:
a stop state pointed at the cycle plays the run on the spot until its
transition times out, which is a second of jogging in place after letting go.

`/Game/Animation/BS_Locomotion` keeps the pack's authored sample positions: X is
lean (-1 to 1), Y is speed in cm/s, with clips at **180 (walk), 500 (jog) and
950 (run)**.

Those are the speeds the clips were actually captured at, and that is what stops
the feet sliding — the character's speeds should be matched to the blend space,
not the other way round. Currently `WalkSpeed` is 500 (lands exactly on the jog
sample) and `SprintSpeed` is **950**, exactly the run sample — at 850 a sprint
sat 78% of the way from jog to run and never arrived.

Nothing drives the character below 500, so the walk clip is never reached; a
walk modifier or analogue input would open up the lower half of the space.

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

- **Aiming** — see *Editor steps, C* below. A full aim strafe set rather than an
  upper-body layer, so the legs read the strafe too.
- **Inertialization.** Setting the state machine's transitions to use
  inertialization rather than a standard blend removes the last of the pops on
  fast direction changes.

---

## Driving the graph from C++

`ABP_Operative` is parented to **`UEOOperativeAnimInstance`**
(`Source/ExecutiveOps/Character/EOOperativeAnimInstance.h`), which computes
everything the graph needs in `NativeThreadSafeUpdateAnimation`.

### Why

The pack's graph fed its own variables from a **`Cast To ThirdPersonCharacter`**
— the pack's demo character. Our pawn is `BP_Operative`, so that cast always
fails, and every variable on its success path sat frozen at zero:

| Frozen variable | Effect |
|---|---|
| `Speed` | the blend space never left the idle sample — no jog, no run |
| `SpeedRequiredForLeap` | 0, so every takeoff cleared it and took the `LeapStart → LeapLoop` branch: a 2.4s wind-up into a 2s dive loop, which is the "uncontrolled, slow" fall |
| `LateralSpeed`, `SpeedWhenStopping` | no lean, wrong stop foot |

`IsInAir?` alone was correct — it comes from a separate `GetMovementComponent →
IsFalling` chain that does not go through the cast.

### What the anim instance exposes

| Property | Meaning | Feeds |
|---|---|---|
| `GroundSpeed` | cm/s | `Speed` |
| `LeanAmount` | −1..1 from turn rate, eased | `LateralSpeed` (the blend space's X axis is a lean, not cm/s) |
| `StoppingSpeed` | speed held while accelerating, frozen on release | `SpeedWhenStopping` |
| `bAccelerating`, `bMoving`, `bInAir` | — | `IsAccelerating`, `IsMoving`, `IsInAir?` |
| `LeapSpeedThreshold` | 1200, above run speed on purpose | `SpeedRequiredForLeap` |
| `MoveDirection` | travel relative to facing, −180..180 | the aim strafe blend space |
| `bIsAiming`, `bIsSprinting`, `bIsSliding` | character state | the Aim state's transitions |
| `FallHeight`, `TimeFalling`, **`bIsLongFall`** | how far the drop has gone | the jump → dive transition |

`bIsLongFall` is the fall fix: a takeoff always plays the controlled jump, and
only a fall that goes past `LongFallHeight` (350cm) or `LongFallTime` (0.6s)
switches to the dive. The pack decided that once, on takeoff, from horizontal
speed; this decides it while falling, from distance.

Property names deliberately do not collide with the graph's own variables, so
the nine transition rules that reference those by name are untouched. The event
graph sets the Blueprint variables from the C++ ones — which is the rewiring
below.

Until the rewiring is done the graph runs on defaults the prepare script sets on
it (`SpeedRequiredForLeap` = 1200), which is enough to stop jumps diving. It is
not enough to make the operative run: `Speed` still needs feeding.

---

## Editor steps

Python cannot create nodes or wire pins. Everything else above is done. What
follows is the node work, in `/Game/Animation/ABP_Operative` unless stated.

### A. Feed the graph from C++ (fixes run, lean, stops)

In the **Event Graph**:

1. Find `Cast To ThirdPersonCharacter`. Delete it, and delete every node that
   was wired to its `As Third Person Character` output. Leave `TryGetPawnOwner`
   and the `IsValid → GetMovementComponent → IsFalling → SET IsInAir?` chain —
   that one works.
2. On the `Event Blueprint Update Animation` exec line, after `SET IsInAir?`,
   add four **SET** nodes and feed each from a **Get** of the matching C++
   property (right-click → search the property name; they are on `self`):

   | SET | ← Get |
   |---|---|
   | `Speed` | `Ground Speed` |
   | `Lateral Speed` | `Lean Amount` |
   | `Speed When Stopping` | `Stopping Speed` |
   | `Speed Required for Leap` | `Leap Speed Threshold` |

   And two more, which are **not optional**: every way out of `Idle` and into
   `StartMoving` / `WalkJogRun` is gated on them, so with these unset the
   operative feeds `Speed` correctly and still never leaves the idle pose.

   | SET | ← Get |
   |---|---|
   | `Is Moving` | `Moving` |
   | `Is Accelerating` | `Accelerating` |

3. Compile. No transition rule should need touching.

Checked live: with `Speed` wired and these two missing, the graph reads
`Speed=500 IsMoving=F IsAccelerating=F` while running and sits in
`Idle/Movement = Idle`.

### B. Jump becomes a dive only after falling far enough

In the **AnimGraph**, open the jump state machine (the one containing
`JumpStart`, `JumpLoop`, `LeapLoop`):

1. Drag from the `JumpLoop` state to the `LeapLoop` state to create a transition.
2. Open its rule and set **Can Enter Transition** to a Get of **`Is Long Fall`**.
3. Give the transition a blend of about 0.3s so the change of pose is not a cut.
4. Compile. `LeapLoop`'s existing exits to the landing states already handle
   coming down.

### C. Aim locomotion

You author the blend space; the graph then gets one state.

1. Create **`/Game/Animation/BS_AimStrafe`** — Blend Space, our skeleton
   (`UE4_Mannequin_Skeleton` under OpenWorldAnimset). Horizontal axis
   **Direction** −180..180, vertical axis **Speed** 0..500.
2. Samples, all from `/Game/OpenWorldAnimset/Animations/Pistol/`:

   | Direction | Speed | Clip |
   |---|---|---|
   | any | 0 | `Pistol_aim_Idle` |
   | 0 | 500 | `Pistol_strafe_fwd` |
   | 45 / −45 | 500 | `Pistol_strafe_fwd_right45` / `Pistol_strafe_fwd_left45` |
   | 90 / −90 | 500 | `Pistol_strafe_right` / `Pistol_strafe_left` |
   | 135 / −135 | 500 | `Pistol_strafe_bwd_right45` / `Pistol_strafe_bwd_left45` |
   | 180 (and −180) | 500 | `Pistol_strafe_bwd` |

   Put all nine loops in one sync group so the feet stay in phase across
   directions.

   The pistol strafes carry no sync markers, and Direction flips from −90 to
   +90 the instant A becomes D, so an unsmoothed blend space cuts between the
   two clips mid-stride: a blink on every reversal. The prepare script sets the
   asset's **Target Weight Interpolation Speed** to 6/s, which eases the sample
   weights rather than the axis and keeps the ±180 wrap intact. Leave it set.

   The same script turns on **Force Root Lock** on every clip in the space.
   None of the pistol strafes are in-place: the root travels about 270cm over
   a loop. The pack ships the straight ones locked and the four diagonals not,
   so a 45° strafe carried the mesh off the capsule and snapped it back on
   release.
3. In the `Idle/Movement` state machine add a state **`Aim`** playing
   `BS_AimStrafe`, with **Direction** ← Get `Move Direction` and **Speed** ←
   Get `Ground Speed`.
4. Transitions: `Idle/Movement`-side states → `Aim` on `Is Aiming`; `Aim` →
   `Idle/Movement` on `NOT Is Aiming`. Blend ~0.2s both ways.
5. Compile.

`SetAiming` already pins the body to the camera and turns A/D into strafes, so
`MoveDirection` reads exactly as the blend space expects: 0 running toward the
crosshair, ±90 strafing, 180 backpedalling.

### Then

Run `./Scripts/run_tests.ps1`, and check each with `ShowDebug ANIMATION`:
sprint sits on the `Run_IP` sample; a jump plays `Jump_Fall_IP` and only a
drop from the aircraft switches to `Leap_Fall_IP`; aiming shows the `Aim` state
with `BS_AimStrafe` driving.

---

## Things that will bite

- **The self-test poses clips directly.** `PlayAnimation` switches the component
  to single-node playback, and nothing switches it back, so the drift check used
  to disable the graph for every phase after it. It now restores the previous
  animation mode. Anything else that calls `PlayAnimation` on the operative owes
  the same courtesy.
- **Verify with the engine, not by eye.** `ShowDebug ANIMATION` prints the live
  node tree — which graph instance is running, which state, which clip, and the
  slot's weight. A still frame cannot tell a working idle from a reference pose;
  that readout can.

---

## Fallback

`UsesDirectAnimationPlayback()` returns true only when the mesh is in
single-node mode, so a project whose animation assets have not been prepared —
no packs imported, no Animation Blueprint — still animates through the old
direct path. The guard still uses it; only the operative has been moved over.
