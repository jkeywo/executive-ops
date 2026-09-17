# Executive Ops

A cyberpunk operative prototype: fly to a site, drop in, complete an objective on
foot, extract. This file names the concepts that recur across the codebase so that
code, docs and design language stay in step.

## The loop

**Mission**:
One run of the loop, from selecting a site to extraction or failure. Owned by
`UEOMissionSubsystem` as a linear state machine.
_Avoid_: level, run, sortie

**Site**:
A place on the city map a mission can be flown to. Selecting a site is separate
from starting the mission.
_Avoid_: target, location, POI

**Deployment sequence**:
The segment of a mission between committing to the drop and the operative landing
under control. Owns the hover hold, the drop, the abort path and the recovery
timeout.
_Avoid_: insertion, spawn, transition

**Extraction**:
The return leg: the aircraft arrives, the operative boards, the mission closes out.
_Avoid_: exfil, pickup, evac

**Hover volume**:
The region above a site inside which deployment is permitted. Entering it is what
makes the drop available.
_Avoid_: trigger, drop zone

## The actors

**Operative**:
The player character on foot.
_Avoid_: player, soldier, agent, character

**Aircraft**:
The player's craft. Flown directly, and also arrives scripted for extraction.
_Avoid_: ship, vehicle, VTOL

**Guard**:
A hostile that perceives, investigates and engages the operative.
_Avoid_: enemy, AI, NPC

## Verbs

**Traversal**:
The vault, mantle and climb family — a scanned obstacle crossed along an authored
arc. Distinct from ordinary movement.
_Avoid_: parkour, climbing, mantling (as a category name)

**Slide**:
A committed low-friction ground movement, entered from a sprint.
_Avoid_: crouch-slide, dive

**Takedown**:
A silent close kill from behind an unaware guard. Deliberately distinguished from
lethal damage so it does not trigger a guard's reaction to being shot.
_Avoid_: stealth kill, assassination, execution

## Presentation

**Feedback event**:
A named moment the game reports — a shot, an impact, a mission transition — which
the feedback module turns into sound, effects, shake, haptics and screen state.
_Avoid_: cue, effect, juice, trigger

**Preset**:
The authored data bound to one feedback event: which sound, which effect, which
shake, at what scale.
_Avoid_: config, profile

**HUD state**:
The snapshot of everything the interface needs, gathered once per frame from the
world and consumed by the widgets. The single seam between game and presentation.
_Avoid_: view model, HUD data, UI state
