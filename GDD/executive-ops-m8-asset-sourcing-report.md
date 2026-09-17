# Executive Ops — M8 Asset Sourcing Report

Response to `executive-ops-m8-polish-shopping-brief.md`.
Researched 2026-09-17. Prices are Fab UK listings in GBP, checked on the day; Fab discounts rotate.

Project engine: **UE 5.8** (`ExecutiveOps.uproject`). Every recommendation below is checked for 5.8 support or flagged.

---

## 0. What you already own

### 0.1 Local Unreal projects — `C:\Users\John\Documents\Unreal Projects`

| Project | Contents | M8 value |
|---|---|---|
| `DynamicLocomotion` | Dynamic Locomotion, Fighting Animset Pro, Open World Animset, Death Animations (MoCap) | Animation only. **Zero audio, zero VFX, zero decals** — no `.wav`, no `NS_*`, no Niagara assets anywhere in either project. |
| `HoldingProject` | PoseAnimations | None for M8. |

The greybox project already has `FightingAnimsetPro` and `OpenWorldAnimset` migrated into `Content/`. So the *animation* side of takedown/parkour is covered; **the entire M8 shopping brief is genuinely unbought.**

### 0.2 Fab library (entitlements, via launcher vault cache)

Owned but **not yet downloaded** (the vault holds manifests only — each needs a Fab download before use):

| Owned pack | M8-relevant contents | Verdict |
|---|---|---|
| **Big Niagara Bundle** | `NS_Dust`, `NS_DustActive`, `NS_DustLow_Active`, `NS_LightDust`, `NS_SparksSwarm`, `NS_SparksSwarmColor`, `NS_ShockWave`, `NS_Jet`, `NS_Jets`, `NS_Relativistic_Jet`, `NS_Sprite_Engine_with_Jet`, `NS_Flying_Trails`, plus a full Hologram pack (`NS_HologramCylinder`, `NS_Ring`, `NS_CityHologramm`) | **Covers roughly 60% of the Niagara list.** Dust for hard landings and hover wash, sparks for slide scrape and metal impacts, jets for VTOL thrust, trails for high-speed streaks, holograms for diegetic UI and terminals. Buy nothing in these categories. |
| **Chameleon Post Process** | Post-process stack with LUTs, layered blend modes, LOOK preset data tables, recipes, widget layers | **Covers section 6 outright.** Use for the neon graphic-novel grade plus the damage vignette, desaturation pulse and speed emphasis. No screen-effects purchase needed. |
| **Explosions Builder** | Particles, VF, mats, meshes, demo room | Out of scope per brief §3 ("no giant explosions"). Ignore for M8. |
| **VFX Grenade Pack** | Grenade FX, plus `Reflections_Spark_Cue` + `reflect_spark_01–05` audio, `GrenadeCameraShake` | Minor: the five spark sounds and the camera shake blueprint are a usable reference. Otherwise defer. |
| Animsets (Dynamic Locomotion, Fighting Animset Pro, Open World Animset, Strike A Pose, Death Animations) | Animation | Already in project or available. |

### 0.3 Engine content

UE 5.8 Starter Content on this machine is **textures only** — no `Audio/` or `Particles/` folders. Do not plan around Starter Content sounds.

---

## 1. Paragon — what it actually gives you

Verified on the Fab listing for *Paragon: Twinblast* (Epic Games, **Free**, Standard License):

- **UE version support is now `4.19–4.27 and 5.0–5.8`** (listing last updated 23 June 2026). The old "Paragon is UE4-only" problem is gone — these import into 5.8 directly.
- Each hero pack contains the character model, skins, animations, **FX**, and animation Blueprints.
- **Critical limitation: Paragon ships voice-line sound cues only. There are no attack, ability, impact or weapon SFX in the packs.** Do not plan any of the audio brief around Paragon.
- Licence restriction: you may not use the *Paragon* trademark to advertise or name the game.

**Recommended Paragon pulls for M8:**

| Pack | Use |
|---|---|
| **Paragon: Twinblast** | Gunslinger hero — dual-pistol animation set and gun FX; closest match to the operative's pistol verb. Cyberpunk-tagged. |
| **Paragon: Lt. Belica** | Sci-fi and cyber energy FX, clean readable silhouettes; useful FX donors for the neon treatment. |
| **Paragon: Wraith / Murdock** | Ranged-weapon FX and muzzle-adjacent effects. |
| Any hero pack | Voice cue banks — a thin but free source of guard pain, effort and death vocals *if* you accept MOBA-flavoured line reads. |

Treat Paragon as a **VFX and animation donor, not an audio source**. Its FX are largely UE4-era Cascade; UE 5.8 still ships the **Cascade to Niagara Effects Converter** plugin, but Cascade is deprecated and scheduled for removal, so convert anything you keep rather than shipping Cascade systems.

---

## A. Recommended minimal purchase / source list

| Category | Asset / pack | Source | Cost | M8 use | Notes |
|---|---|---|---|---|---|
| **Everything audio (spine buy)** | **Pro Sound Collection** — Gamemaster Audio | Fab | **£44.82** | Pistol, bullet impacts, footsteps and foley, UI family, alarms and beeps, sci-fi, punches (takedown), whooshes, voice | 8,076 SFX. Single purchase covering **guns, bullets, footsteps, foley, UI, sci-fi, punches, explosions, voice**. Listing declares 4.9–5.4 only — sound waves and cues migrate forward to 5.8 without trouble, but expect a version prompt and a re-save. Best value on the board. |
| **Vocals (free)** | **Human Vocalizations** — Gamemaster Audio | Fab | **Free** (permanent Epic sponsored content, Standard License) | Takedown pain vocal, guard pain and death, operative exertion breaths | 1,034 vocals, male and female: attacking groans, death screams, painful grunts, various efforts. Covers brief §2.5 and most of §2.6. Also included inside Pro Sound Collection — grab free first, skip if buying the collection. |
| **Aircraft** | **Aircraft Engines Sound Pack** — Magic Sound Effects | Fab | **£13.44** (personal) / £22.40 (professional) | Hover idle, thrust loop, high-speed layer, boost, braking, flyby | UE **5.0–5.8** confirmed. 13 aircraft with fast/slow accel and decel, low/high RPM loops, and 26 pass-bys. That accel/decel plus RPM structure is exactly the speed-blendable layering the brief asks for. Fills the one category Pro Sound Collection does not cover well. |
| **Pistol (alternative / top-up)** | **Modern Pistols SFX** — Sound Kajiya | Fab | **£4.46–£8.95** | Pistol shot, slide and mag mechanical action, outdoor tail | UE **4.27, 5.0–5.8**. 25 gunshots, 14 outdoor tails, reload mechanics, 48kHz. Buy *instead of* Pro Sound Collection if going minimum-spend; the layered two-to-three-source shots read as "precise and expensive" rather than huge. |
| **Footsteps (alternative)** | **Essential Footsteps SFX** — Sound Kajiya | Fab | **Free (personal) / £3.57 (professional)** | Concrete, metal and gravel/soft surface families, plus stop/jump/land | UE **4.27, 5.0–5.8**. 272 files, 10 surfaces, and it already separates **land** from **jump** — matching the brief's landing-light/landing-hard split. Three-surface requirement met with room to spare. |
| **Raw audio (free)** | **Sonniss #GameAudioGDC bundle** (2026 plus archive) | gdc.sonniss.com | **Free** | Wind and airflow layers, aircraft scrape, deployment mechanism, ricochet, whizz, cloth and armour movement | Royalty-free, no attribution, commercial use permitted, perpetual. Best source for the odd one-shots nobody sells as a pack. **Cost is curation time, not money** — budget an afternoon to cut and name about 20 files. Licence explicitly forbids AI/ML training use. |
| **Weapon VFX** | **Stylized Shooting VFX Niagara (Muzzle · Bullet Trail · Impact)** — Fateloom | Fab | **£18.81** | Muzzle flash, tracer trail, impact | Stylised rather than photoreal — the closest fit to the neon graphic-novel treatment. Native Niagara. |
| — alternative | **Niagara Muzzle Flash & Ejection VFX** — Sidearm Studios | Fab | £26.88 | Muzzle flash, shell ejection | Higher fidelity, more realistic; pick this only if the comic treatment moves toward grounded. |
| — budget alternative | **Muzzle Flashes Bundle** — MajiqFX | Fab | £13.44 | Muzzle flash only | Cheapest credible muzzle option; pair with the owned `NS_SparksSwarm` for impacts. |
| **Aircraft VFX** | **VTOL Thrusters FX** — Shogun Games | Fab | **£10.75** (from £21.50) | VTOL exhaust and thrust cones, deployment launch burst | The one aircraft-VFX shape the Big Niagara Bundle's space jets do not quite give you — they read as engine plumes, not ducted VTOL wash. Optional: `NS_Jets` plus `NS_DustActive` can fake it. |
| **Decals** | **Decals VOL.6 — Bullet Holes** — Dekogon Studios | Fab | **£8.95** | Bullet mark, hard surface and metal | Tier 2. Clean, neutral, no gore. Alternative: *Bullet Hole Decals* (Underground Workshop) at £4.92 with 50% off, but Mature-tagged. |
| **Guard barks** | **Soldier Character PRO Voice Pack** — Datvoice | Fab | **£13.44** | Idle, suspicious, investigating, confirmed, attack, pain, death | Tier 2. Buy only if Paragon's MOBA voice lines and Human Vocalizations' non-verbal grunts prove too unreadable for the detection states. Try free-first. Mashmashu's *Hunter / Daniel / Lauren – Military Soldier Voice Pack* (£22.40 each) are the higher-quality step up. |
| **Characters / anim donors** | **Paragon: Twinblast**, **Paragon: Lt. Belica** | Fab | **Free** | Pistol anims, gun FX, cyber FX donors | UE 5.0–5.8. No attack SFX — VFX and anims only. |

**Deliberately not recommended** (fails brief §12 or §3): Explosions Builder content, FX Variety Pack (Kakky, free — but it is a fire/thunder/water *magic* pack), Realistic Starter VFX Pack Vol 2 (a free tier exists, but it is blood/explosion/destruction-led and the professional licence is £26.88), any thousand-sound multi-weapon-class library, any full cyberpunk HUD kit.

---

## B. Reuse map

| Asset family | Flight | Deploy | Sprint/parkour | Takedown | Pistol | Guard fire | Player damage | Detection | Extraction |
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| Pro Sound Collection | ● beeps/alarms | ● mechanism | ● foley | ● punches, foley | ● guns | ● guns | ● impacts, UI | ● UI/alarm | ● UI, voice |
| Human Vocalizations (free) | | ● effort | ● exertion | ● pain/effort | | | ● pain | | ● effort |
| Aircraft Engines Sound Pack | ● core | ● approach | | | | | | | ● hover + approach |
| Modern Pistols SFX | | | | | ● core | ● core | | | |
| Essential Footsteps SFX | | ● landing | ● core | ● contact | | | | | ● boarding steps |
| Sonniss bundle | ● wind/scrape | ● mechanism | ● cloth | ● accent | ● tail | ● whizz/ricochet | ● near-miss | | ● wash |
| **Big Niagara Bundle (owned)** | ● jets, trails | ● dust burst | ● dust, sparks | ● shockwave accent | ● sparks | ● sparks | | ● holograms | ● hover wash |
| **Chameleon (owned)** | ● speed grade | ● punch-in | ● speed lines | ● hit accent | ● recoil flash | | ● vignette/flash | ● alert grade | ● capture grade |
| Stylized Shooting VFX | | | | | ● core | ● core | ● incoming tracers | | |
| VTOL Thrusters FX | ● exhaust | ● launch burst | | | | | | | ● hover wash |
| Bullet decals | | | | | ● marks | ● marks | ● near-miss reading | | |
| Paragon (Twinblast/Belica) | | | | ● anims | ● anims/FX | ● anims | ● reaction anims | | |

Highest-leverage single purchase: **Pro Sound Collection**, touching nine verbs out of nine. Highest-leverage already-owned asset: **Chameleon**, also nine of nine, at zero spend.

---

## C. Gaps — build in-house, do not buy

1. **Camera feedback presets (brief §5).** Nine distinct shake profiles plus FOV impulse curves. Purchased shake packs are generic, and the brief explicitly forbids one generic shake. Reference `GrenadeCameraShake` in the owned VFX Grenade Pack for structure, then author your own. **Build.**
2. **Haptics (brief §7).** Force-feedback effect assets are a twenty-minute authoring job in-editor. **Build.**
3. **Screen effects (brief §6).** Chameleon is already owned and covers the whole list. **Configure, do not buy.**
4. **Detection, damage-direction and action-prompt UI (brief §8).** UMG plus a handful of simple materials; the owned hologram Niagara set gives diegetic flavour. Buying a HUD kit at M8 is explicitly deferred. **Build.**
5. **Comic impact accent for takedown.** No pack fits the neon graphic-novel treatment cleanly. Author one Niagara sprite burst plus one post-process flash via Chameleon. **Build.**
6. **The feedback routing layer itself.** There is no audio or VFX code in `Source/ExecutiveOps/` yet — no sound assets, no Niagara references. Before any import, M8 needs a data-driven feedback component so effects are *data*, not hard-coded spawn calls. **Build first.**
7. **Aircraft deployment and extraction mechanism sounds.** Not sold as a coherent pack at this scale. Cut them from Sonniss. **Curate.**

---

## D. Estimated total cost

| Tier | Contents | Cost |
|---|---|---|
| **Minimum viable** | Human Vocalizations (free) + Paragon packs (free) + Sonniss (free) + Modern Pistols SFX £4.46 + Essential Footsteps SFX £3.57 (professional) + Aircraft Engines Sound Pack £13.44 + owned Big Niagara and Chameleon | **about £21.47** |
| **Comfortable** | Pro Sound Collection £44.82 + Aircraft Engines £13.44 + Stylized Shooting VFX £18.81 + the free packs + owned packs | **about £77.07** |
| **Comfortable plus Tier 2** | the above + VTOL Thrusters FX £10.75 + Dekogon bullet decals £8.95 + Soldier Voice Pack £13.44 | **about £110.21** |
| **Optional upgrades** | Professional licence uplifts (Aircraft Engines +£8.96, Modern Pistols +£4.49), Mashmashu soldier voice packs £22.40 each, Sidearm muzzle and ejection £26.88 | +£30–£90 |

**Recommendation: the Comfortable tier at roughly £77.** Pro Sound Collection removes four separate purchases and gives variation depth the minimum tier cannot.

### Licence warning

Several Fab listings show a price *range* ("From free to £X", "From £4.46 to £8.95"). That is the **Personal versus Professional** licence split — the free or cheap tier is not valid above Fab's revenue threshold for commercial work. If Executive Ops is intended commercially, budget the professional tier on **Essential Footsteps SFX (£3.57)**, **Modern Pistols SFX (£8.95)** and **Aircraft Engines Sound Pack (£22.40)**. Human Vocalizations and Paragon are flat Free under the Standard License, with no uplift.

---

## E. Integration order

1. **Build the feedback framework first.** A feedback component plus data asset (sound, Niagara, camera preset, haptic and screen effect per gameplay event) wired to the existing events in `EOAircraftPawn`, `EOOperativeCharacter`, `EOTraversalComponent`, `EOGuardCharacter`, `EOHealthComponent` and `EOMissionSubsystem`. Importing assets before this exists produces hard-coded spawn calls you will rip out.
2. **Download the owned packs.** Big Niagara Bundle and Chameleon are entitlements sitting undownloaded in the vault. Free, immediate, and they cover dust, sparks, jets, holograms and the whole post-process brief. Verify 5.8 import before spending anything.
3. **Free audio pass.** Human Vocalizations plus Paragon voice cues. Wire takedown, guard pain and death, and player damage. Cheapest readability win available.
4. **Pistol and impacts.** Pro Sound Collection (or Modern Pistols SFX), then muzzle Niagara, then decals. This is the verb the player triggers most and judges hardest.
5. **Movement audio.** Essential Footsteps SFX, three surface families only — concrete, metal, soft. Land-hard and land-light come from the pack's own land/jump split.
6. **Aircraft audio.** Aircraft Engines Sound Pack, blended by throttle and speed rather than switched. Slowest to tune, so do it once the simpler verbs are proven.
7. **Aircraft and operative VFX.** Owned `NS_Jets`, `NS_DustActive` and `NS_SparksSwarm` first; buy VTOL Thrusters FX only if the owned jets read wrong.
8. **Camera presets and haptics.** Nine profiles, authored against the now-working audio and VFX so intensities are tuned in context rather than in isolation.
9. **Chameleon screen-effect pass.** Grade, damage vignette, speed emphasis, alert state. Last, because it sits on top of everything else.
10. **UI cues and indicators.** UI sound family from Pro Sound Collection, plus minimal UMG. Defer the HUD.

---

## Sources

- Fab: [Paragon: Twinblast](https://www.fab.com/listings/9fa88852-5711-42e1-94fa-2491498a64da) · [Pro Sound Collection](https://www.fab.com/listings/baaa5ef9-1238-414d-97a1-f1c667aa1ec6) · [Human Vocalizations](https://www.fab.com/listings/98259abf-477f-4015-8abe-2c9f62eaefdb) · [Aircraft Engines Sound Pack](https://www.fab.com/listings/968b9092-493c-4425-9970-9932f8ce08d1) · [Modern Pistols SFX](https://www.fab.com/listings/3a3822f6-8f07-4139-8d53-e93bc760f3f5) · [Essential Footsteps SFX](https://www.fab.com/listings/d8833d76-3270-41d5-8fd0-efeae4057750)
- [Epic Games — Paragon assets release](https://www.unrealengine.com/en-US/paragon)
- [Epic Developer Community — Paragon assets have voice cues only, no ability SFX](https://forums.unrealengine.com/t/sound-fx-missing-from-the-paragon-assets-is-a-tragedy/109364)
- [UE 5.8 docs — Cascade to Niagara Effects Converter plugin](https://dev.epicgames.com/documentation/en-us/unreal-engine/cascade-to-niagara-effects-converter-plugin-for-unreal-engine)
- [Sonniss GDC Game Audio Bundle](https://gdc.sonniss.com/) · [licence](https://sonniss.com/gdc-bundle-license/)
