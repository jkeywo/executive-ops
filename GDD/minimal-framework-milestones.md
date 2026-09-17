# Minimal Framework Milestones

## Goal

Produce the smallest playable framework that answers:

> **Is it enjoyable to fly through a cyberpunk city, deploy into a compact space, move through it quickly, kill or evade a guard, and extract back to the aircraft?**

The framework should use:
- greybox environments;
- production-intent character animations;
- minimal but functional VFX/audio;
- no final art requirement;
- no procedural mission generation;
- no campaign;
- no progression;
- no territory simulation;
- no cloak escalation;
- no hacking;
- no rival operatives.

Once the complete loop works, development changes from **adding features** to **refining the basic verbs**.

---

# M0 — Project Skeleton

## Goal

Establish the UE5 project and technical foundations without building game systems that are not immediately needed.

## Implement

- UE5 project.
- Source control.
- Enhanced Input.
- Basic game/state structure.
- Player controller.
- Third-person character using chosen production character rig.
- Import owned parkour animation packs.
- Empty flight map.
- Empty mission map.
- Simple debug UI.
- Basic restart/reset command.

## Technical decisions

Establish interfaces for:
- aircraft control;
- player deployment;
- mission start;
- mission complete;
- extraction.

Do not build generalized frameworks beyond what the next milestones use.

## Done when

The project reliably boots into a test map and the player can possess either the operative or a placeholder aircraft.

---

# M1 — Fly

## Goal

Make the aircraft enjoyable enough to move around before building the city around it.

## Implement

A greybox VTOL with:
- forward/backward movement;
- strafing;
- vertical movement;
- yaw;
- hover;
- acceleration/deceleration;
- high-speed flight;
- transition between precision hover and faster forward flight;
- chase camera.

Cockpit camera can be extremely basic or deferred slightly if it slows development.

## Environment

Use:
- boxes;
- towers;
- pillars;
- tunnels/gaps;
- altitude changes.

This is a **flight playground**, not yet the city.

## Minimal feedback

- engine loop;
- acceleration audio;
- very simple thruster VFX;
- speed indicator.

## Done when

Flying around obstacles for five minutes is controllable and understandable without fighting the controls.

Do not polish handling yet beyond removing obvious problems.

---

# M2 — City Flight Greybox

## Goal

Test the aircraft against something resembling the eventual game's scale.

## Implement

One tiny greybox city district containing:
- several tall buildings;
- street canyons;
- rooftop levels;
- elevated infrastructure;
- open flight routes;
- tighter optional routes;
- one obvious mission building.

Target **minutes, not kilometres**, of environment.

This map should primarily answer questions about:
- aircraft scale;
- desired flight speed;
- building spacing;
- useful vertical range;
- camera readability;
- streaming requirements.

## Navigation

Add:
- mission marker;
- simple world map or tactical map;
- waypoint;
- mission-site highlight.

The city map can initially be little more than a top-down representation of this district.

## Done when

The player can select the test mission and comfortably fly from somewhere else in the district to its deployment zone.

---

# M3 — Deployment and Ground Transition

## Goal

Create the signature transition between the two halves of the game.

## Implement

At the mission site:

1. Enter designated hover volume.
2. Aircraft stabilises or assists the player into position.
3. Select one fixed deployment socket.
4. Trigger deployment.
5. Short launch/drop sequence.
6. Possess operative.
7. Land at the mission start point.
8. Ground controls become active.

For now:
- no cloak;
- no insertion choice;
- no mission timer;
- no aircraft support.

The aircraft can simply remain parked/frozen in the expected location.

## Done when

Flying into the mission and becoming the operative feels like one continuous game rather than changing levels through a debug menu.

---

# M4 — Ground Movement Playground

## Goal

Get the movement verbs represented using the **real animation set**.

## Build

A small greybox route containing:
- open sprinting;
- low vault;
- high vault/mantle;
- climb;
- jump/drop;
- slide;
- corners;
- elevation changes;
- at least two alternative paths through part of the space.

Use the owned animations now rather than placeholders.

## Controls

Initial target:
- normal movement;
- sprint;
- crouch if needed;
- contextual parkour input;
- slide;
- jump where appropriate.

Traversal assistance should favour successful movement over precision input.

## Do not implement yet

- perception mode;
- grapple;
- dash cyberware;
- wall running unless already trivial with the animation set;
- elaborate procedural parkour detection.

Use explicit traversal objects/sockets if that gets the result working faster.

## Done when

The player can repeatedly traverse a short route without animation failures, unexpected stops, or needing precise platforming.

---

# M5 — One Guard

## Goal

Add the smallest complete stealth/combat problem.

## Guard behaviour

One guard:
1. walks a fixed patrol;
2. can see the player;
3. becomes alerted;
4. shoots at the player;
5. pursues sufficiently to remain dangerous;
6. can lose the player if appropriate.

Do not build the final alert architecture yet.

The AI needs only enough behaviour to test the player's verbs.

## Player options

### Assassination

Approach valid target from close range.

Press takedown.

Fast contextual takedown animation.

Guard dies.

### Gun

Equip one pistol.

Support:
- aim;
- shoot;
- hit reaction;
- guard death.

### Being shot

The guard can:
- acquire player;
- aim;
- fire;
- damage;
- kill player.

Use a simple health model.

No cover shooter systems.

## Bodies

Dead guard remains where they fell.

Nothing else needs to react to the body because only one guard exists.

## Done when

The exact same encounter can naturally produce:
- successful stealth kill;
- gun kill;
- botched stealth attempt;
- short firefight;
- player death.

---

# M6 — Minimal Mission

## Goal

Turn the movement playground and guard into an actual mission.

## Map

One fixed greybox mission arena.

Include:
- insertion point;
- several parkour obstacles;
- one patrol route;
- one guard;
- one objective;
- one extraction point.

The objective can be deliberately trivial:

**Reach terminal and press interact.**

There is no need to prove objective mechanics yet.

## Mission flow

```text
INSERT
  ↓
Reach objective
  ↓
Kill / avoid guard
  ↓
Interact with objective
  ↓
Extraction activates
  ↓
Reach extraction
```

The guard does not have to be killed.

## Done when

The player can complete the whole ground mission repeatedly without debug intervention.

---

# M7 — Extraction and Return to Flight

## Goal

Complete the first full game loop.

## Implement

At the extraction zone:

1. Player activates extraction.
2. Aircraft approaches/appears over the extraction point.
3. Short pickup animation/transition.
4. Player returns to aircraft control.
5. Mission marked complete.
6. Player can fly away through the city.

Initially the craft does not need sophisticated autonomous navigation.

A constrained scripted arrival is preferable to implementing a general aircraft AI.

## Complete framework loop

```text
CITY MAP
    ↓
FLY
    ↓
APPROACH MISSION
    ↓
DEPLOY
    ↓
PARKOUR / STEALTH
    ↓
ASSASSINATE / SHOOT / EVADE
    ↓
OBJECTIVE
    ↓
EXTRACT
    ↓
FLY AWAY
```

## Framework Complete

At this point **stop adding major features**.

The project now contains every basic verb required to assess the concept.

---

# M8 — Basic Verb Game-Feel Pass

This is the important phase.

The goal changes from:

> "Can the player do it?"

to:

> "Does doing it feel excellent?"

Work one verb at a time.

## Flight

Tune:
- acceleration;
- deceleration;
- yaw rate;
- banking;
- vertical thrust;
- hover assistance;
- momentum;
- camera lag;
- FOV;
- near-miss sensation;
- controller/mouse response.

Add only the feedback necessary to judge it:
- engine pitch;
- wind;
- thruster response;
- camera movement;
- subtle particles;
- speed effects.

## Deployment

Tune:
- approach;
- hover snap/assistance;
- input flow;
- animation duration;
- launch velocity;
- camera;
- landing;
- transition back to player control.

Target:

**quick, violent, confident.**

Deployment should become something the player enjoys seeing repeatedly.

## Sprint / Parkour

Tune:
- acceleration;
- animation transitions;
- contextual detection;
- vault distances;
- mantle timings;
- input buffering;
- camera;
- landing recovery;
- ability to chain actions.

The test is not animation realism.

The test is:

> **Can I decide where I want to go and get there rapidly without wrestling the traversal system?**

## Takedown

Tune:
- acquisition range;
- target selection;
- snapping;
- animation speed;
- impact;
- camera behaviour;
- sound;
- hit pause if useful;
- immediate return to movement.

It should be extremely quick.

Avoid elaborate executions.

## Gunplay

Keep mechanically simple, but make the basic pistol satisfying.

Tune:
- aiming;
- recoil;
- shot timing;
- impact reaction;
- audio;
- muzzle flash;
- enemy reaction;
- lethality.

Do not add weapon complexity as a substitute for making the basic gun feel good.

## Being Detected

Even with one guard, prototype clear feedback for:
- noticing something;
- acquiring the player;
- confirmed detection;
- losing sight.

This lays the foundation for the later stealth system.

## Being Shot

Tune:
- readable incoming fire;
- damage feedback;
- hit reaction without excessive movement interruption;
- lethality.

The player should immediately understand:

> **I need to move, not settle into this firefight.**

## Extraction

Tune:
- call timing;
- aircraft arrival;
- anticipation;
- boarding;
- sound;
- camera;
- return to flight.

Extraction should feel like the release at the end of the ground sequence.

---

# M9 — Perception Mode

Only add the signature perception mechanic **after the uncontrolled basic verbs feel reasonable**.

Implement the simplest version:
- hold/toggle ability;
- world heavily slows;
- player movement restricted;
- look around normally;
- highlight guard;
- highlight useful traversal objects;
- show guard awareness;
- release to return immediately to normal time;
- limited charge;
- recharge.

Do not initially implement:
- action queues;
- route planning;
- complex hacking;
- elaborate tactical UI.

The key test is whether perception mode actually creates the desired rhythm:

**look → decide → move violently → disappear → look again.**

If it does not, change the mechanic before building systems around it.

---

# Milestone Boundary

At the end of M9 the project should still be extremely small:
- one greybox district;
- one aircraft;
- one operative;
- one fixed mission site;
- one objective;
- one guard;
- one pistol;
- one takedown;
- a handful of parkour verbs;
- one insertion;
- one extraction;
- perception mode.

But it contains the **complete experiential thesis of the game**.

Only after this point should development move into:
- multiple guards;
- proper alert propagation;
- procedural mission configuration;
- cloak pressure;
- rival operatives;
- hacking;
- aircraft support;
- factions;
- Authority/Heat;
- progression;
- campaign generation.

---

# Development Rule

Until M9 is convincing:

> **New content is almost never the answer.**

If the prototype feels weak, modify:
- timing;
- control;
- acceleration;
- camera;
- animation transitions;
- input assistance;
- hit response;
- sound;
- feedback;
- encounter geometry.

The purpose of this framework is to make the small number of verbs cheap to iterate on before the rest of the game starts depending on them.
