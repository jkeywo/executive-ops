# Laying out WBP_HUD

A walk through the Widget Blueprint designer, slot by slot, using the geometry
the Canvas HUD already has - so the widget reads the same as what it replaces
and nothing moves when you switch over. The slot *names and types* are the
contract in `Docs/HUD-Widgets.md`; this is how to arrange them.

Two facts shape everything below:

- The Canvas HUD was **authored at 720p and scales by height**. Every number
  here is a 720p design pixel. Build the Blueprint at a 1280×720 design size
  (Designer → screen size dropdown → 1280×720) and let DPI scaling do the rest;
  the project's default DPI curve scales by the shortest side, which is height
  in every aspect you care about.
- **Colour is set every frame from the value's tone**, so leave every text and
  bar colour at its default. Anything you colour in the designer gets
  overwritten on the first frame. Style is typography, spacing and panels.

## The frame

```
   40                                                                40
   ┌──────────────────────────────────────────────────────────────────┐
34 │ ┌─ 1 asset ─────┐      ┌──── 2 bearing (480) ────┐   ┌─ 3 standing ┐ │
   │ │   320 wide    │      │  compass tape + heading  │   │  320 wide   │ │
   │ └───────────────┘      │  directive               │   └─────────────┘ │
   │                        └──────────────────────────┘                    │
236│ ┌─ 4 feed ──────┐                                                       │
   │ │  rows...      │                 5 focus                               │
   │ │               │            (reticle, arcs r=92,                       │
   │ └───────────────┘             prompt below)                             │
   │                                                                          │
   │ ┌─ 6 self ──────┐      ┌─── 7 charge (360) ───┐   ┌─ 8 commit ────┐    │
   │ │  124 tall     │      │  hover bar + hint     │   │   124 tall    │    │
   │ └───────────────┘      └──────────────────────┘   └───────────────┘    │
   └──────────────────────────────────────────────────────────────────┘
      bottom row: 40 from the bottom edge     charge band: 120 from bottom
```

Root: a **Canvas Panel**. Every slot is a child of it, anchored to the edge or
centre it belongs to, so the left and right columns stay on their own edges at
any aspect and the centre band stays centred.

| Slot | Anchor | Position (720p px) | Size |
|---|---|---|---|
| 1 Asset | top-left | x 40, y 34 | 320 × ~110 |
| 2 Bearing | top-centre | x −240 (centred), y 34 | 480 × ~120 |
| 3 Standing | top-right | x −360 (40 from right), y 34 | 320 × ~110 |
| 4 Feed | top-left | x 40, y 236 | 320 × ~300 |
| 5 Focus | centre | centred on 0,0 | ~260 × 260 |
| 6 Self | bottom-left | x 40, y −164 (40 + 124 from bottom) | 320 × 124 |
| 7 Charge | bottom-centre | x −180 (centred), y −120 | 360 × ~40 |
| 8 Commit | bottom-right | x −360, y −164 | 320 × 124 |

Padding inside every panel is **13 px**. The key chips (the `F` / `E` badges)
are text with 6 px horizontal and 3 px vertical padding on a filled border.

## Typography

Three sizes carry the whole interface. Set them once as three Text styles and
reuse them:

| Role | Use |
|---|---|
| **Caption** | small, uppercase, tracked out - labels like AIRFRAME, INTEGRITY, SCAN |
| **Body** | the default - values, prompts, feed rows |
| **Headline** | large - the one number or word a slot is about: the distance, DEPLOY, 000 |

Captions sit above their value. In the Canvas version a caption and its value
are a two-line unit with the caption ~20 px above the headline's baseline.

## Slot by slot

Each panel is a **Border** (the frame) containing a **Vertical Box** with
13 px padding. Panels 1, 3, 6 and 8 are boxed; 4, 5 and 7 draw without a
border. Everything named in bold must be named exactly that in the hierarchy.

### 1 Asset - top left, boxed

```
[Caption] AssetCaption        AIRFRAME / OVERWATCH
[Body]    AssetMode           HOVER · FLIGHT · INBOUND · STANDBY
[Headline] AssetHeadline      STATION KEEPING, or "340 M" on foot
[Bar]     AssetFill           thrust - only meaningful while flying
```

Caption and mode share a row (caption left, mode right-aligned - a Horizontal
Box with the mode's slot set to Fill and right-justified). Headline below. The
bar last, 6 px tall, full width. On foot the bar reads 0; that's expected.

### 2 Bearing - top centre, unboxed

```
        [compass tape - Blueprint-drawn, see below]
              [Headline] HeadingText   000
   [Caption] DirectiveLabel   DIRECTIVE / OBJECTIVE
   [Body]    Directive        APPROACH HARBOUR RELAY
```

The tape spans **90 degrees** across the 480 px width - a tick every 10°,
major every 30°, cardinals N E S W. The heading badge sits centred under it
with a filled background so it reads over the ticks. Directive label and text
are centred beneath. The tape and its two markers (site, extraction) are
Blueprint-drawn from `Heading`, `SiteRelativeBearing`, `ExtractionRelativeBearing`
and their `bHas…` flags - see "Blueprint-drawn" below.

### 3 Standing - top right, boxed

```
[Caption] SITE STATE   [Body] SiteState        NO CONTACT … ALERTED
[Caption] CERTAINTY    [Body] CertaintyText    72%
[Bar]     CertaintyFill
```

Two caption/value rows, then a bar. The whole slot's tone tracks the worst
guard, so this is the panel that goes red.

### 4 Feed - left column, unboxed

```
[Caption] FeedCaption       SCAN - 2 RETURNS  /  CONTACT LOG
[Vertical Box] FeedRows     (rebuilt from GetViewModel().Feed each frame)
```

Each row: label left, tag beside it in caption style, distance right-aligned
(`340M`), and for contacts a thin fill bar under it showing detection. Give
the rows box a fixed row height (~26 px) so the list doesn't jump. Build the
rows in **On Applied** - see below.

### 5 Focus - dead centre, unboxed

A small reticle at the centre. Around it, at **radius 92 px**, the threat
arcs. Beneath it, ~60 px down, the prompt:

```
[Horizontal Box] PromptGroup   (collapsed when there's no prompt)
   [Body, chip] PromptKey     F
   [Body]       Prompt        TAKEDOWN
```

Leave `PromptGroup` visible in the designer; the widget collapses it when
`Prompt` is empty. While flying the reticle is replaced by a marker projected
from `FlightReticleWorld` - Blueprint-drawn.

### 6 Self - bottom left, boxed, 124 tall

Two layouts share this panel; the widget shows one and collapses the other:

```
[Horizontal Box] FlightReadouts        three equal columns
   SPEED KM/H  →  [Headline] SpeedText
   ALT M       →  [Headline] AltitudeText
   V/S M/S     →  [Headline] VerticalSpeedText

[Vertical Box] GroundReadouts
   [Caption] INTEGRITY   [Body] IntegrityText   82% - SPRINTING
   [Bar]     IntegrityFill                       6 px tall
```

Captions above headlines in each column. Put both boxes in the same Overlay
so they occupy the same space.

### 7 Charge - bottom centre, unboxed, 120 from the bottom

```
[Horizontal Box] ChargeGroup   (collapsed on foot)
   [Caption] HOVER    [Bar] HoverFill    [Caption, right] ChargeHint  HOLD LEFT SHIFT
```

One row, 360 wide: caption, the bar filling the middle, hint right-aligned.

### 8 Commit - bottom right, boxed, 124 tall

```
[Caption]  CommitCaption     INSERTION / EXTRACTION
[Body, chip] CommitKey       F / E / (empty)     [Headline] CommitHeadline   DEPLOY
[Caption]  CommitDetail      the blocker, or "180 M"
```

Key chip and headline share a row. When `CommitKey` is empty the chip should
collapse - easiest is to bind the chip's visibility to whether its text is
empty, in On Applied.

## Blueprint-drawn

Four things a text or a bar can't do. Implement **On Applied** (it fires after
every `Apply`, with the same values) and read **Get View Model**:

- **Feed rows.** Clear `FeedRows`' children, then for each entry in `Feed` add
  a row widget (make a small `WBP_HudFeedRow` with label/tag/distance/bar).
  Rebuilding ~5 rows a frame is fine at this scale.
- **Threat arcs.** For each entry in `Threats`: an arc centred at
  `RelativeBearing` degrees clockwise from straight up, radius 92, span 22°
  (34° when the tone is Alarm), thickness scaling with `Detection`. Either
  paint them in **On Paint** with *Draw Lines* around the circle, or rotate an
  arc image per threat. Colour from *Colour For Tone*.
- **Compass tape.** In On Paint: for each 10° tick within ±45° of `Heading`,
  x = 240 + (Δ / 45) × 240 across the 480 px band; taller ticks every 30°;
  cardinals at 0/90/180/270. A diamond at `SiteRelativeBearing` and a smaller
  one at `ExtractionRelativeBearing` when their flags are set.
- **Flight reticle.** While `bHasFlightReticle`: *Project World To Screen*
  on `FlightReticleWorld`, place a marker there, clamped to a 220 px circle
  round the centre so it never leaves the reticle's neighbourhood.

## Checking it

Assign `WBP_HUD` as **Viewport Widget** (and **Panel Widget**) in Project
Settings → Game → Executive Ops - HUD, then play. A blank slot is one that's
unplaced or misnamed - the compiler won't warn, because every slot is optional,
so check spelling against `Docs/HUD-Widgets.md` first. Toggle to first person
(`V`) to see the same layout on the cockpit glass.
