# Executive Ops — M8 Polish Implementation Brief for an LLM

## Purpose

This document instructs an LLM or coding agent how to perform the **M8 Basic Verb Game-Feel Pass** for *Executive Ops*.

The project should still look like a greybox after this pass.

Success means the basic verbs already feel like they belong to the final game.

The LLM must improve **response, readability, impact, and flow** before adding new gameplay systems.

---

# 1. Scope

Polish only these current verbs:

1. Flight
2. Deployment
3. Sprint / parkour
4. Takedown
5. Pistol fire and hit response
6. Guard gunfire
7. Player damage
8. Detection feedback
9. Extraction

Do not add:

- procedural missions;
- rival operatives;
- cloak pressure;
- hacking;
- multiple weapons;
- multiple enemy classes;
- progression;
- campaign logic;
- territory simulation;
- civilians as gameplay;
- dogfighting.

---

# 2. Core Principle

Every gameplay action should answer four questions clearly:

1. **Did the game accept my input?**
2. **What happened?**
3. **What state am I in now?**
4. **What should I do next?**

Decorative effects come only after these are clear.

---

# 3. Feedback Stack

For each verb, consider the following layers:

- controls;
- animation;
- sound;
- particles;
- world reaction;
- camera;
- screen feedback;
- haptics;
- UI/state feedback.

Not every action needs every layer.

Use the smallest combination that communicates the action strongly.

---

# 4. Shared Feedback Architecture

Create a reusable **Feedback Preset** system.

A preset should be data-driven and may contain:

- Niagara system;
- one or more sounds;
- camera impulse/shake profile;
- FOV impulse;
- controller haptic profile;
- optional screen/post-process pulse;
- optional decal;
- optional hit-stop/local timing accent;
- intensity scalar;
- lifetime;
- attenuation settings.

Example event names:

- `Aircraft_Accelerate`
- `Aircraft_BrakeHard`
- `Aircraft_Collision`
- `Deploy_Launch`
- `Deploy_Land`
- `Parkour_Vault`
- `Parkour_Slide`
- `Takedown_Impact`
- `Pistol_Fire`
- `Pistol_Hit`
- `Player_Damaged`
- `Guard_DetectConfirmed`
- `Extraction_Call`
- `Extraction_Board`

Use a simple intensity category:

- Subtle
- Normal
- Strong
- Signature

Do not hardcode effects separately into every gameplay class if one shared feedback dispatcher can handle them.

---

# 5. Flight Polish

## Goal

Flight should feel responsive at low speed and powerful at high speed.

## Tune first

- acceleration curve;
- deceleration;
- strafe response;
- vertical thrust;
- yaw rate;
- banking;
- hover assist;
- high-speed inertia;
- mouse/controller response;
- dead zones;
- camera lag;
- camera recenter rules.

Do this before adding large amounts of VFX.

## Audio

Blend by speed and thrust:

- idle/hover loop;
- thrust layer;
- high-speed layer;
- wind layer.

Add one-shots for:

- hard acceleration;
- hard braking;
- lateral thruster burst;
- collision;
- scrape.

## VFX

Use:

- thruster/exhaust;
- hover wash near surfaces;
- subtle speed streaks at high speed only;
- collision sparks.

## Camera

Use:

- mild FOV expansion with speed;
- small camera lag under acceleration;
- slight turn lead where useful;
- brief FOV compression under hard braking;
- directional collision impulse.

Avoid continuous random screenshake.

## Done when

The aircraft communicates:
- speed;
- weight;
- thrust;
- braking;
- collision

without requiring the player to stare at the HUD.

---

# 6. Deployment Polish

## Goal

Deployment should feel like the operative is being fired into the mission space as a corporate weapon.

## Sequence

### Anticipation
- mechanical restraint/release sound;
- small framing movement if useful;
- brief visual readiness cue.

### Launch
- sharp mechanical/propulsion transient;
- pressure/vapour VFX;
- strong but brief camera impulse;
- short FOV expansion.

### Descent
- wind rush;
- optional speed streaks;
- stable readable camera.

### Landing
- hard surface impact sound;
- dust/debris burst;
- short camera compression/impulse;
- immediate control return.

## Rule

Do not make deployment cinematic at the expense of responsiveness.

## Done when

The sequence feels:
- fast;
- violent;
- controlled;
- repeatable;
- satisfying.

---

# 7. Sprint / Parkour Polish

## Goal

The player should decide where they want to go and get there without fighting the traversal system.

## Prioritise

- input buffering;
- forgiving contextual detection;
- good traversal target selection;
- momentum preservation;
- fast animation transitions;
- minimal landing lockout;
- consistent camera continuity;
- reliable chaining.

## Required action phases

Every traversal action should read as:

**approach → commitment → contact → release**

## Audio

Implement:

- footsteps by broad material;
- vault hand contact;
- vault foot contact;
- landing;
- slide;
- climb/mantle contact;
- armour/cloth movement.

## VFX

Use sparingly:

- hard landing dust;
- slide sparks on metal;
- minor contact dust only where contextually appropriate.

Do not emit dust on every action.

## Camera

Use tiny controlled impulses rather than generic shake.

## Done when

The player can chain movement without unexpected dead time and can understand when a contextual parkour action has been accepted.

---

# 8. Takedown Polish

## Goal

The takedown should be the strongest ground action in M8 while remaining short.

## Required

- reliable target acquisition;
- clean snap/alignment;
- immediate input acknowledgement;
- fast animation;
- strong contact frame;
- layered impact audio;
- optional tiny impact particle;
- enemy body reaction;
- small directional camera impulse;
- immediate return to locomotion.

## Timing

Test a very short local impact accent.

Avoid making every takedown globally freeze the world.

Perception mode will later own most of the game's temporal identity.

## Rule

Do not extend the animation because it looks cool.

If the takedown interrupts the speed fantasy, shorten it.

---

# 9. Pistol Polish

## Goal

The single pistol should feel convincing enough that the game does not need weapon complexity to create feedback.

## On fire

Trigger:

- muzzle flash;
- weapon recoil animation;
- slide/mechanical motion where available;
- directional camera kick;
- short haptic pulse;
- layered shot audio.

## On world hit

Trigger material-specific response:

### Hard surface
- dust/chip VFX;
- impact sound;
- decal.

### Metal
- sparks;
- metallic impact sound;
- decal.

### Character / armour
- compact hit effect;
- hit reaction;
- impact sound.

## Audio

Layer:
- shot transient;
- mechanism;
- environment tail.

Use subtle variation.

## Rule

Camera response should be **recoil**, not arbitrary screenshake.

---

# 10. Guard Gunfire Polish

## Goal

Enemy fire must feel dangerous even when it misses.

## Implement

- muzzle flash;
- gun report;
- bullet whizz / near-miss sound;
- nearby material impact;
- sparks or dust;
- occasional visible tracer if needed for readability;
- directional origin readability.

## Done when

A near miss causes the player to move even before health is lost.

---

# 11. Player Damage Polish

## Goal

Damage should tell the player to move without taking control away.

## Required

- directional damage indicator;
- short impact sound;
- brief animation flinch;
- short haptic response;
- temporary vignette or directional edge flash;
- clear health/state response.

## Avoid

- sustained blur;
- long stun;
- large full-screen red overlays;
- aggressive chromatic aberration;
- heavy camera shake.

The player needs spatial awareness to escape.

---

# 12. Detection Polish

## Goal

Detection must communicate escalating certainty, not just binary hidden/seen.

Even with one guard, implement clear stages.

Suggested minimal states:

1. Unaware
2. Noticed something
3. Investigating
4. Confirmed detection
5. Lost contact

## Communicate through

### Animation
- head turn;
- posture shift;
- weapon readiness.

### Audio
Distinct cues for:
- suspicion;
- investigation;
- confirmation;
- safe/lost contact.

### UI
- directional awareness indicator;
- simple state icon or meter.

### Screen
At confirmed detection only:
- restrained pulse or vignette.

Do not shake the camera because the player was detected.

---

# 13. Extraction Polish

## Goal

Extraction should feel like the release and payoff at the end of the ground sequence.

## Call

- clear confirmation sound;
- extraction state UI;
- distant aircraft audio begins.

## Approach

- spatial aircraft engine ramps;
- hover/thruster VFX;
- environmental wash if practical;
- visual anticipation.

## Boarding

- short strong mechanical contact sound;
- brief camera impulse;
- transition animation;
- immediate return to aircraft control.

## Done when

Ground → aircraft feels as coherent as aircraft → ground.

---

# 14. Screen Effects Language

Use a deliberately small vocabulary.

## FOV impulse
Use for:
- aircraft acceleration;
- deployment;
- possibly weapon recoil.

## Directional camera impulse
Use for:
- pistol recoil;
- takedown contact;
- hard landing;
- collision.

## Camera shake
Reserve for:
- large physical impact;
- aircraft collision;
- exceptional events.

## Vignette
Use for:
- damage;
- severe danger.

## Speed streaks
Use for:
- high aircraft speed only.

## Comic overlay
Optional:
- deployment;
- takedown;
- major impact.

Do not use:
- constant chromatic aberration;
- constant film grain;
- full-screen flashes for routine actions;
- heavy motion blur as a crutch.

---

# 15. Accessibility / User Settings

Expose at minimum:

- Camera Shake: Off / Reduced / Full
- Screen Effects Intensity: Reduced / Full
- Haptics: Off / On
- Motion Blur: Off / On
- FOV control where applicable

Design the feedback system so these settings can scale effects centrally.

Do not bolt accessibility controls on afterward.

---

# 16. Implementation Order

Polish one complete verb at a time.

Recommended order:

1. Flight
2. Deployment
3. Sprint
4. One vault
5. Slide
6. Takedown
7. Pistol fire
8. Pistol hit
9. Guard fire / near miss
10. Player damage
11. Detection
12. Extraction

For each verb:

1. tune controls;
2. fix animation;
3. add audio;
4. add VFX;
5. add camera feedback;
6. add UI/state feedback;
7. add haptics if useful;
8. compare before/after;
9. remove any effect that does not improve readability or feel.

---

# 17. Review Checklist Per Verb

The LLM must verify:

## Responsiveness
- Does input produce a visible response immediately?
- Is input buffering needed?
- Is there dead time?

## Readability
- Can the player understand what happened?
- Can they understand the new state?

## Impact
- Does the action have an appropriate transient?
- Is the strongest moment visually and audibly clear?

## Recovery
- How quickly can the player act again?
- Does polish accidentally slow the game?

## Context
- Does the effect make sense on this surface / material / action?

## Noise
- Is any feedback redundant?
- Are multiple effects fighting for attention?

## Accessibility
- Can strong shake / flashes / haptics be reduced or disabled?

---

# 18. Success Criteria for M8

M8 is complete when the game is still recognisably a greybox but:

- flight feels responsive and fast;
- deployment feels like a signature action;
- sprinting and parkour flow cleanly;
- takedowns feel decisive;
- the pistol feels satisfying;
- enemy fire feels dangerous;
- taking damage is clear without destroying visibility;
- detection is readable;
- extraction feels like a payoff.

The player should not need final environment art for the loop to feel deliberate.

---

# 19. Hard Rule Against Scope Creep

If a polish task suggests adding a new gameplay system, ask:

> Can the same improvement be achieved through timing, control, animation, audio, VFX, camera, or UI?

If yes, improve the existing verb.

Do not solve weak game feel by adding content.
