# Executive Ops — M8 Polish Shopping Brief for an LLM

## Purpose

This document instructs an LLM or coding/design agent to identify the minimum asset library required for the **M8 Basic Verb Game-Feel Pass** in *Executive Ops*.

The objective is **not** to make the game look finished.

The objective is to give every core verb enough audio, VFX, camera, UI, and tactile feedback that the greybox already feels deliberate and satisfying.

The current M8 verbs are:

1. Flight
2. Deployment
3. Sprint / parkour
4. Takedown
5. Pistol fire / hit
6. Guard firing at player
7. Player taking damage
8. Detection / alert
9. Extraction

Do not source assets for systems that are not yet in M8.

---

# 1. General Procurement Rules

## Prefer

1. Existing owned assets.
2. Free or inexpensive marketplace/store assets.
3. Packs with broad reuse across several verbs.
4. Assets with permissive commercial licenses.
5. Unreal Engine-ready assets where practical.
6. Modular source files rather than highly specific one-off effects.

## Avoid

- giant cinematic VFX packs;
- photoreal assets that clash with the stylised comic treatment;
- weapons or sound packs tied to dozens of unused weapon classes;
- assets that require major bespoke code merely to function;
- highly destructive environment systems;
- gore systems;
- large music libraries at this stage;
- final UI packages;
- anything whose primary value is content quantity rather than usefulness to the core verbs.

## Visual Style

The game uses a **neon graphic-novel / comic-book** baseline:

- dark graphite and gunmetal;
- restrained cyan and magenta accents;
- crisp silhouettes;
- stylised rather than photoreal presentation;
- readable effects;
- minimal visual noise.

VFX must remain readable against both dark interiors and bright cyberpunk signage.

---

# 2. Required Audio Asset Library

The LLM should source the smallest practical set that covers the following.

## 2.1 Aircraft

Required:

- hover / idle loop;
- thrust / acceleration loop;
- high-speed engine layer;
- wind / airflow layer;
- boost or strong acceleration one-shot;
- hard braking one-shot;
- lateral thruster burst;
- aircraft collision / scrape;
- deployment mechanism;
- extraction approach / hover presence;
- extraction capture / boarding;
- optional flyby or pass-by layer.

Target:
- approximately 3–4 loops;
- approximately 6–10 one-shots.

The engine layers should be blendable by speed/throttle rather than used as discrete binary sounds.

---

## 2.2 Operative Movement

Required broad surface families:

- hard floor / concrete;
- metal;
- soft / composite / interior.

Required action sounds:

- footsteps;
- landing light;
- landing hard;
- slide;
- vault hand contact;
- vault foot contact;
- climb / mantle contact;
- cloth / armour movement;
- optional short exertion breaths.

Do not buy a giant 100-surface footstep library unless it is cheap and easy to use.

Three convincing surface groups are enough for M8.

---

## 2.3 Pistol

Required:

- pistol shot transient;
- weapon mechanical action;
- indoor / neutral tail;
- optional outdoor tail;
- dry-fire / empty click if trivial;
- reload sounds only if reload exists at M8;
- shell/mechanical detail if useful.

Variation:

- at least 2–4 subtle variants for the main firing sound or layered components.

The pistol should sound precise and expensive rather than huge.

---

## 2.4 Bullet Impacts

Required material families:

- hard wall / concrete;
- metal;
- character / armour.

Optional:

- glass, only if already relevant.

Required supporting sounds:

- ricochet;
- near-miss / bullet whizz;
- bullet impact near player.

---

## 2.5 Takedown

Required:

- body impact;
- armour / clothing contact;
- short mechanical or cybernetic accent;
- restrained enemy pain/breath vocal;
- optional low-frequency transient layer.

No gore-specific sound set is required.

---

## 2.6 Guard

Required vocal categories:

- idle or minimal patrol effort;
- suspicious;
- investigating;
- confirmed detection;
- attack / combat bark;
- pain;
- death.

Keep the set small.

The purpose is state readability, not authored dialogue depth.

---

## 2.7 UI / State Cues

Required:

- action available;
- action unavailable;
- detection rising;
- detection confirmed;
- lost contact / safe again;
- objective complete;
- extraction available;
- mission complete;
- damage warning;
- aircraft state change.

Prefer a coherent small sci-fi UI sound family.

---

# 3. Required Niagara / VFX Library

The LLM should source or plan to create approximately 8–12 reusable effects.

## Essential

### Aircraft
- VTOL exhaust / thrust;
- hover wash / dust disturbance;
- high-speed streaks;
- collision sparks.

### Operative
- hard landing dust / debris;
- slide scrape / sparks for metal;
- optional foot contact dust.

### Weapon
- pistol muzzle flash;
- metal impact sparks;
- hard-surface impact dust/chip;
- character/armour hit effect.

### Optional
- short deployment launch burst;
- extraction hover wash;
- subtle comic impact accent for takedown.

Do not source:
- giant explosions;
- destruction packs;
- dismemberment;
- magic effects;
- screen-filling lens effects.

---

# 4. Decals / Persistent Marks

Required:

- bullet mark — hard surface;
- bullet mark — metal;
- optional scorch/scrape decal;
- optional temporary skid/landing mark.

Persistence should be limited by a simple budget.

Do not implement persistent world damage systems.

---

# 5. Camera Feedback Assets / Presets

These are primarily implementation presets rather than purchased assets.

Create reusable profiles for:

1. Aircraft acceleration
2. Aircraft hard braking
3. Aircraft collision
4. Deployment launch
5. Hard landing
6. Pistol recoil
7. Takedown impact
8. Player damage
9. Extraction capture

Each preset should define some subset of:

- camera positional impulse;
- camera rotational impulse;
- FOV impulse;
- duration;
- intensity;
- attenuation;
- optional haptic intensity.

Do not use one generic shake for all events.

---

# 6. Screen / Post-Process Effects

The LLM should implement or source one small reusable screen-effects material or post-process stack capable of:

- temporary vignette;
- directional damage edge flash;
- brief exposure or desaturation pulse;
- speed emphasis;
- optional stylised comic hit overlay;
- optional subtle edge accent.

Avoid by default:

- sustained chromatic aberration;
- heavy blur;
- full-screen white flashes;
- constant film grain;
- constant speed lines;
- aggressive camera distortion.

All screen effects should expose intensity settings.

---

# 7. Haptics

No external purchase required.

Create controller feedback profiles for:

- pistol shot;
- being hit;
- hard landing;
- aircraft collision;
- deployment launch;
- extraction boarding.

No constant engine rumble is required initially.

---

# 8. UI Graphics

M8 only needs minimal functional visual feedback.

Required:

- detection indicator;
- damage direction indicator;
- action prompt;
- objective state;
- extraction state;
- basic flight speed / throttle indication;
- optional action cooldown/progress if needed.

Do not purchase a full cyberpunk HUD kit unless it is exceptionally cheap and can be used selectively.

The final HUD is not an M8 dependency.

---

# 9. Asset Search Criteria

For every candidate asset, report:

- asset name;
- source/store;
- price;
- license;
- UE version compatibility where relevant;
- whether it is plug-and-play or requires conversion;
- exact M8 uses;
- whether it duplicates an existing candidate;
- file/asset count only where useful;
- whether style modification is likely required.

Score candidates on:

- usefulness to M8;
- breadth of reuse;
- integration effort;
- visual/audio fit;
- cost.

Do not score on raw quantity.

---

# 10. Purchase Priority

## Tier 1 — Buy / source first

- aircraft engine/thrust audio;
- pistol audio;
- impact audio;
- guard vocal set;
- core Niagara impact/muzzle/aircraft effects;
- movement/footstep audio.

## Tier 2 — Add if cheap/useful

- UI sound family;
- hover wash;
- bullet whizz;
- extra landing/slide effects;
- decals.

## Tier 3 — Defer

- music;
- final UI;
- bespoke cinematics;
- large environment ambience packs;
- advanced destruction;
- dogfighting effects;
- rival operative-specific effects;
- cloak effects;
- hacking effects.

---

# 11. Deliverable Format

The LLM should return:

## A. Recommended minimal purchase/source list

A concise table with:

| Category | Asset / pack | Source | Cost | M8 use | Notes |

## B. Reuse map

For each purchased asset family, list every verb it can support.

## C. Gaps

Identify which assets are better created in-house rather than purchased.

## D. Estimated total cost

Give:
- minimum viable spend;
- comfortable spend;
- optional upgrades.

## E. Integration order

Recommend the order in which assets should be imported and wired into the M8 feedback framework.

---

# 12. Hard Scope Rule

Do not recommend an asset unless it materially improves one of the current M8 verbs:

> flight → deploy → move → takedown / shoot → survive → extract

If it does not improve that loop, defer it.
