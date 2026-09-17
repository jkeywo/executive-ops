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

## F. Free-only alternative stack

Every paid recommendation above has a free substitute except aircraft engine audio. Licences checked individually — all listed here are flat **Free** under Fab's Standard License (valid commercially), not personal-tier-free, unless noted.

| Paid pick | Free substitute | Source | Trade-off |
|---|---|---|---|
| **Pro Sound Collection** £44.82 | No single equivalent — replaced by the five free packs below plus Sonniss curation | — | This is the real loss. You give up one coherent, consistently-mastered library and 8,076 files of variation depth, and take on the job of level-matching packs from six different authors. Budget curation time instead of money. |
| **Modern Pistols SFX** £4.46–8.95 | **Free Weapon Sound Effects** — SoundLab1 | Fab, Free | 6 weapons × 3 shot variations, includes handgun plus silenced variants, reload mechanics and handling sounds. Authored from scratch, no AI or third-party content. Meets the brief's "2–4 subtle variants" requirement exactly. Genuinely competitive — this is not a downgrade. |
| **Aircraft Engines Sound Pack** £13.44 | **Sonniss #GameAudioGDC** archive + freesound.org (CC0 filter) | Free | **The one real gap.** Nothing on Fab gives a free, speed-blendable aircraft loop set. Sonniss archives are raw location recordings organised by contributor, so you are hunting through ~200GB for turbine and airflow material, then building the accel/decel/RPM layering yourself. Several hours of work versus £13.44. *This is the one I would still pay for.* |
| **Stylized Shooting VFX Niagara** £18.81 | **NEON WEX — Free Muzzle Flash FX** — Night447 Studios | Fab, Free | Niagara muzzle flashes, UE5-native, and the listing explicitly grants free personal *and commercial* use. Neon-styled, so arguably a closer fit to the graphic-novel treatment than the paid pick. The seller offers a paid professional licence purely as a tip jar. |
| — impacts half of the same job | **Niagara Examples Pack** — **Epic Games** | Fab, Free | **The strongest free find.** Epic's own pack: bullet impacts, sparks, trails, smoke, hit dissolves, animation-notify footstep effects, pings and markers — authored to best practice with Effect Types, Niagara Data Channels and lightweight emitters. Listed for **UE 5.7** against your 5.8, so verify import. Worth taking even if you buy everything else, as a reference for how the feedback layer should be structured. |
| **VTOL Thrusters FX** £10.75 | Owned **Big Niagara Bundle** (`NS_Jets`, `NS_DustActive`) | Owned | Already the fallback named in section A. No new cost. |
| **Decals VOL.6 — Bullet Holes** £8.95 | Author in-engine | — | A bullet decal is a masked texture on a deferred decal material. Two textures and one material instance, half an hour. Nothing here justifies a purchase. |
| **Soldier Character PRO Voice Pack** £13.44 | **Male Character Vocalizations SFX Pack LITE** — Hove Audio | Fab, Free | 118 vocalizations across combat (14), dialogue (27), emotional states (32), health conditions (26), physical states (19). UCS-compliant naming. Covers the brief's guard state readability requirement and the takedown pain vocal. Pair with the free **Human Vocalizations** (Gamemaster) for breadth. |
| **UI sound family** (inside Pro Sound Collection) | **SCI-FI UI SOUND EFFECTS PACK** — Hove Audio | Fab, Free | 100+ bleeps, 29 clicks, 12 rings, 31 glitches, 6 impacts. Coherent sci-fi family, which is exactly what §2.7 asks for. Delivered dry — the listing suggests adding delay and reverb. Alternatives: *Interface & Item Sounds Pack* (Daydream Sound), *UI SFX Free Pack* (Skril Studio), *TII 3Step SCI-FI Audio Package*. |
| **Essential Footsteps SFX** £3.57 pro tier | **Footsteps Mini Sound Pack** — Mechanics Mechanics | Fab, flat Free | Avoids the personal-vs-professional licence question entirely. Smaller than the Kajiya pack; if you want the 272-file, 10-surface version for commercial use, £3.57 is the cheapest line on the whole list and not worth engineering around. |
| **Comic impact accent** (listed as build-in-house) | **Easy Impact Frames** — Vefects | Fab, Free | UE **4.27, 5.0–5.8**. Stylised anime-style hit frames, drag-and-drop, customisable. Covers the takedown comic accent from §3 without authoring it. Reclassify this gap from "build" to "free download". |
| **Foot contact dust** (optional, §3) | **Niagara Footstep VFX** — Sidearm Studios | Fab, Free | Optional brief item, now free. |

### Revised cost

- **All-free stack: £0**, with aircraft audio curated from Sonniss.
- **£0 stack plus the one purchase worth keeping: £13.44** (Aircraft Engines Sound Pack, personal) or **£22.40** (professional).
- Compare against **£77.07** for the Comfortable tier in section D.

The free stack costs roughly a day of curation and level-matching that Pro Sound Collection would have saved. At M8 — a game-feel pass on a greybox, where nothing is final — that trade is reasonable. If audio work later becomes a recurring cost, Pro Sound Collection remains the sane purchase.

### Not a source

Searches for these packs surface sites such as `ue3dfree.com` and `assetfreaks.com` that redistribute paid Fab content as free downloads. Those are unlicensed copies — no valid licence for a commercial project, and repackaged `.uasset`/installer files are a standard malware vector. Everything in this section is a legitimately free asset from its own publisher.

---

## G. Download recheck — later on 2026-09-17

Re-scanned the vault. Most of sections A and F are now **downloaded payloads**, not just entitlements. Project `Content/` is still untouched (1,443 assets, animation only) — nothing has been migrated in yet.

### Downloaded and ready to migrate

| Pack | Size | Verified contents |
|---|---|---|
| **Paragon: Twinblast** | 2.4 GB | 389 FX assets, 484 audio cues |
| **Paragon: Lt. Belica** | 2.0 GB | 524 FX assets, 489 audio cues, separate `Audio/Cues` + `DialogueWaves` + `Wavs` tree |
| **Explosions Builder** | 1.5 GB | (out of M8 scope) |
| **Big Niagara Bundle** | 789 MB | `NS_Jet`, `NS_Jets`, `NS_Four_Jets`, `NS_Sprite_Engine`, `NS_Sprite_Engine_with_Jet`, `NS_RelativisticJet`, plus dust/sparks/shockwave and `BP_Jets_Movement` / `BP_SpriteEngineMovement` driver blueprints |
| **VFX Grenade Pack** | 633 MB | spark cues, `GrenadeCameraShake` |
| **Chameleon Post Process** | 261 MB | see "buried finds" below |
| **Human Vocalizations** | 148 MB | **7 voice actors** — HumanMale A/B/C/D, HumanFemale A/B/C — each with its own `Cues` and `Wavs` trees. More per-character variation than expected; enough to give guards distinct voices. |
| **Easy Impact Frames** | 133 MB | `BP_Impact_Frames_Manager`, camera shake `BP_CS_01`, VFX set, demo character |
| **NEON WEX — Muzzle Flash FX** | 62 MB | **11 Niagara muzzle flashes** (`FXS_NS_MuzzleFlash_00–10`), `FXS_NS_ShotBurst_01/02`, separate Shell and Smoke particle folders, plus its own `Sound/Sfx` folder |
| **Essential Footsteps SFX** | 49 MB | 521 files. Per surface: Walk / Jog / Run / Jump / **Land** / Walk_Stop / Jog_Stop / Run_Stop. Concrete, Dirt, Glass, Gravel, Leaves, Metal, Sand, Slush, Snow |
| **SCI-FI UI SOUND EFFECTS PACK** | 19 MB | 786 files: Clicks, Click_Combos, FX_Sounds, Glitches, Impacts, Rings, Tone1/2/3 |
| **Free Weapon Sound Effects** | 4.5 MB | Handgun: 3 gunshots + silenced variants + **4-stage reload** (slide lock, mag drop, mag insert, slide release) + equip + tails. Same structure for assault rifle and shotgun. |
| Animation packs | — | Dynamic Locomotion, Fighting Animset Pro, Open World Animset, Strike A Pose, Death Animations |

### Still not downloaded

- **Male Character Vocalizations LITE** (Hove Audio, free) — redundant now that Human Vocalizations turns out to carry **seven distinct voice actors** (HumanMale A–D, HumanFemale A–C), each with its own cue tree.

### Downloaded since, and now imported

- **Lyra Starter Game** → `Documents\Unreal Projects\LyraStarterGame`. Its `Content/Audio` carries `Sounds/WhizBys` (16 bullet in/out passes), `Sounds/Impacts`, `Sounds/Weapons` (Pistol, Rifle, Shotgun MetaSounds), `Sounds/Footsteps`, `Sounds/Movement`, `Sounds/UI`, plus `Feedback/CameraShakes` and `Feedback/Haptics` worth reading as reference.
- **Niagara Examples Pack** → `HoldingProject/Content/NiagaraExamples`. `FX_Weapons/Impacts` gives `NS_Impact_Concrete`, `_Metal`, `_Wood`, `_Glass`; plus `NS_MuzzleFlash`, `NS_BulletTracer`, and the `FX_Sparks` set.

All thirteen packs are now imported into `Content/` by `Scripts/import_fab_assets.ps1` and excluded from git.

### Buried finds inside packs already downloaded

- **`T_BulletHole`** — a bullet hole texture, sitting in Chameleon's `Textures` folder. Section F said author the decal yourself; the texture is already on disk. One decal material away from done.
- **`M_EdgeDetect3x3HLSL`** — an edge-detection post-process material in Chameleon. This is the comic-book ink-line look, already owned.
- **`M_Alarm`** — alarm-state post-process material in Chameleon. Maps directly onto the detection-confirmed state.
- **`BP_CS_01`** in Easy Impact Frames and `GrenadeCameraShake` in the VFX Grenade Pack — two worked camera shake examples to pattern the nine presets on.
- **NEON WEX ships its own `Sound/Sfx`** — muzzle audio you were not expecting to get with a free VFX pack.

### Paragon — confirmed by inspection, not description

- **Particles are Cascade** (`P_` prefix), as predicted. Route them through the Cascade-to-Niagara converter.
- **Genuinely useful donors:** `P_TwinBlast_Nitro_HitWorld`, `P_TwinBlast_Nitro_HitCharacter`, `P_TwinBlast_Nitro_Bullet_Trail_Smoke_Spline`, `P_DiveBooster_Kickup_XForward`, `P_DiveBooster_Arms`, `P_NitroActive_CameraFX`. That is hard-surface impact, character impact, bullet trail and thruster kickup dust — four of the brief's weapon and aircraft VFX entries, from a free pack.
- **Audio is dialogue only, and confirmed unusable for guard barks.** Every file is `*_Dialogue.uasset` with MOBA-specific names: `Ability_LowMana`, `Ability_OnCooldown`, `Ability_Q_Engage`, `Ability_UsedLastMana`. There is no detection, patrol or investigation vocabulary anywhere in it.

---

## H. Remaining gaps, sourced beyond Fab

Five gaps survive everything downloaded. Licences verified individually.

### 1. Aircraft / VTOL audio — the one genuine gap

Still nothing. No free pack anywhere gives a speed-blendable VTOL loop set; this is assembly work whichever route you take.

| Source | Licence | What you get |
|---|---|---|
| **Pixabay** (`pixabay.com/sound-effects`) | Pixabay Content License — **commercial use allowed, no attribution required**. You may not resell the files standalone in substantially unchanged form; using them as game audio is fine. | Jet engine, turbine and airplane engine loops, searchable and individually auditioned. The most practical route: pick 4–5 files and layer them. |
| **Sonniss #GameAudioGDC 2026** | Royalty-free, no attribution, perpetual, commercial. **AI/ML training prohibited.** | 7.47 GB, 347+ files from 17 vendors (344 Audio, Epic Stock Media, SoundBits, Just Sound Effects, The Noisery and others). Split ZIPs, Google Drive mirror or torrent. Past years' archive adds ~200 GB. Raw location recordings — no aircraft guarantee, so audition before committing time. |
| **Kenney — Sci-fi Sounds** (`kenney.nl`) | **CC0**, no attribution | 70 sounds, engine material included. Arcade-flavoured rather than grounded — a fallback, not a first choice. |
| **freesound.org**, CC0 filter | CC0 | Individual turbine and airflow recordings. Filter to CC0 specifically; the site mixes CC-BY and non-commercial licences. |

**Honest recommendation: this is still the £13.44 I would spend.** Aircraft is the M8 verb the player spends most time inside, layered audio is the hardest thing to fake from scraps, and Aircraft Engines Sound Pack hands you accel/decel/RPM structured for exactly this.

### 2. Bullet impacts, ricochet and whizz-by → **Lyra Starter Game** (Epic, Free)

The strongest find of this pass. Free, **UE 5.0–5.8**, licensed as *UE-Only Content* — which constrains it to Unreal-based products, and this is one.

Lyra's core weapon, character and gameplay audio is built in **MetaSounds**, including a system that simulates the bullet **whizz-by** effect from weapon fire — brief §2.4's "near-miss / bullet whizz", which nothing else free covers. It also carries footsteps, impacts and UI cues, and the MetaSound graphs are worth reading as reference for the speed-blended aircraft audio you will build in step 6.

Caveat: distributed as a **complete project**, not an asset pack, so you open it separately and migrate assets across rather than adding it to Executive Ops.

### 3. Hard-surface and character impact VFX → already solvable

**Niagara Examples Pack** (Epic, free, listed 5.7) plus the Paragon `P_*_HitWorld` / `P_*_HitCharacter` Cascade donors above. No further sourcing needed.

### 4. Movement foley — slide, vault contact, cloth/armour

Essential Footsteps covers walk/jog/run/stop/jump/land only. Slide, vault hand and foot contact, and cloth movement are missing. **Sonniss** foley libraries and **freesound** CC0 are the routes; a vault hand-slap and a fabric rustle are two of the easier things to find, or to record on a phone. Low risk, low cost, some curation.

### 5. Guard barks — no good free answer

- **Kenney Voiceover Pack** — 90 files, CC0, male and female. Thin and generic.
- **itch.io CC0 voice packs** — the available ones (*Warrior*, *Mage*, *Orc*, *RPG Voice Starter*) are fantasy-flavoured; wrong register for a cyberpunk guard.
- **Human Vocalizations** (owned) covers non-verbal pain, death and effort across seven actors — which may be enough, since the brief asks for *state readability*, not authored dialogue.

Try shipping M8 on non-verbal vocals plus the UI alert family. If detection states still read poorly in playtest, **Soldier Character PRO Voice Pack (£13.44)** is the fix — but that is a decision to make after testing, not before.

### Revised spend after the recheck

**£0 of the section F stack is still outstanding** — it is all downloaded. Remaining considered spend: **£13.44 aircraft audio** (recommended), plus **£13.44 guard barks** only if playtesting demands it.

---

## I. What the M8 pass actually consumed

Implemented in `Source/ExecutiveOps/Feedback/` and bound by `Scripts/m8_build_feedback_presets.py`. See `Docs/M8-Feedback.md`.

| Verb | Audio | VFX | Camera / screen |
|---|---|---|---|
| Flight | *(gap)* | — | FOV impulse on surge and brake |
| Aircraft impact | SciFi UI impacts | Niagara Examples `NS_Impact_Metal`, Big Niagara sparks | `EOShake_AircraftCollision` |
| Deployment | Human Vocalizations effort | Big Niagara `NS_Jets`, `NS_DustActive` | `EOShake_DeployLaunch`, +12° FOV |
| Landing | Essential Footsteps `Land` | Big Niagara dust | `EOShake_HardLanding` |
| Parkour | Human Vocalizations effort, Essential Footsteps metal contact | — | — |
| Slide | Essential Footsteps run-stop | Big Niagara low dust | — |
| Takedown | SciFi UI impacts | Easy Impact Frames | `EOShake_TakedownImpact` + 0.08s local hit-stop |
| Pistol | Free Weapon Sounds handgun | NEON WEX muzzle flash, Niagara Examples impacts | `EOShake_PistolRecoil` |
| Guard fire | Free Weapon Sounds | NEON WEX muzzle | — |
| Near miss | **Lyra WhizBys** | — | — |
| Player damage | Human Vocalizations pain | — | `EOShake_PlayerDamage`, vignette, direction indicator |
| Detection ladder | SciFi UI rings and glitches | — | Restrained vignette on confirm only |
| Extraction | SciFi UI combos and impacts | — | `EOShake_ExtractionBoard` |

**Unused by M8, deliberately:** Explosions Builder, VFX Grenade Pack, Paragon (both packs — animation and Cascade FX donors for later, not for a feedback pass), Chameleon's LUT library. Chameleon contributes `T_BulletHole`, `M_EdgeDetect3x3HLSL` and `M_Alarm` when the post-process pass happens.

**The one gap the pass could not fill:** aircraft engine audio. `Aircraft_Accelerate` and `Aircraft_BrakeHard` ship carrying FOV and camera response only.

---

## Sources

- Free substitutes on Fab: [Niagara Examples Pack (Epic)](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600) · [NEON WEX — Free Muzzle Flash FX](https://www.fab.com/listings/f81d7b34-525f-4700-9794-cb436a71dfb0) · [Free Weapon Sound Effects](https://www.fab.com/listings/1697af22-7e2a-410e-b8c0-88239216520d) · [SCI-FI UI SOUND EFFECTS PACK](https://www.fab.com/listings/d3d9b060-7b69-4130-91d3-c96c2f3cb549) · [Male Character Vocalizations LITE](https://www.fab.com/listings/6199187d-eb1e-4b30-ae3e-a46fece5e83e) · [Easy Impact Frames](https://www.fab.com/listings/15cb7c95-3220-43fe-8d68-c67c73e83eba)
- Fab: [Paragon: Twinblast](https://www.fab.com/listings/9fa88852-5711-42e1-94fa-2491498a64da) · [Pro Sound Collection](https://www.fab.com/listings/baaa5ef9-1238-414d-97a1-f1c667aa1ec6) · [Human Vocalizations](https://www.fab.com/listings/98259abf-477f-4015-8abe-2c9f62eaefdb) · [Aircraft Engines Sound Pack](https://www.fab.com/listings/968b9092-493c-4425-9970-9932f8ce08d1) · [Modern Pistols SFX](https://www.fab.com/listings/3a3822f6-8f07-4139-8d53-e93bc760f3f5) · [Essential Footsteps SFX](https://www.fab.com/listings/d8833d76-3270-41d5-8fd0-efeae4057750)
- [Epic Games — Paragon assets release](https://www.unrealengine.com/en-US/paragon)
- [Epic Developer Community — Paragon assets have voice cues only, no ability SFX](https://forums.unrealengine.com/t/sound-fx-missing-from-the-paragon-assets-is-a-tragedy/109364)
- [UE 5.8 docs — Cascade to Niagara Effects Converter plugin](https://dev.epicgames.com/documentation/en-us/unreal-engine/cascade-to-niagara-effects-converter-plugin-for-unreal-engine)
- [Sonniss GDC Game Audio Bundle](https://gdc.sonniss.com/) · [licence](https://sonniss.com/gdc-bundle-license/)
- Beyond Fab: [Lyra Starter Game](https://www.fab.com/listings/93faede1-4434-47c0-85f1-bf27c0820ad0) (also via the Epic Games Launcher UE Samples tab) · [Kenney audio packs, CC0](https://kenney.nl/assets/category:Audio) · [Pixabay Content License](https://pixabay.com/service/license-summary/) · [freesound.org CC0](https://freesound.org/browse/tags/cc0/) · [OpenGameArt Sci-Fi Sound Effects Library (CC-BY 3.0 — attribution required)](https://opengameart.org/content/sci-fi-sound-effects-library)
