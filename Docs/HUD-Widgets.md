# Building the HUD widgets

The logic is in C++; the layout is yours. `UEOHudWidget` copies a view model
into named widgets each frame and does nothing else, so a Widget Blueprint
parented to it needs only to *contain* widgets with the right names and types.
Anything named below that the Blueprint has not placed is simply skipped -
every slot is `BindWidgetOptional` - so the interface can be built one slot at
a time against a game that keeps working.

Until a widget class is assigned, the Canvas HUD keeps drawing. Assigning one
is the switch: the Canvas slots stand down and the widget draws instead, from
the same state. The screen-flash and vignette stay on Canvas either way; they
are hiding a cut, and that is not a job for a widget.

## Where the values come from

```
world  ->  FEOHUDState  ->  FEOHudViewModel  ->  UEOHudWidget  ->  your Blueprint
          (gathered once)   (strings, fractions,   (copies into      (layout)
                             tones - no widgets)    named slots)
```

If a value you want is not on the view model, add it there
(`UI/EOHudViewModel.h`) - never by reaching into the world from the Blueprint.
That is the seam that keeps the viewport and the cockpit panel showing the same
thing.

## Steps

1. Content Browser → **User Interface → Widget Blueprint**. When asked for a
   parent class, pick **EOHudWidget**. Save it as `/Game/UI/WBP_HUD`.

2. Lay out a Canvas Panel with the eight slots in the positions the design
   gives them:

   ```
   1 asset (top left)    2 bearing (top centre)   3 standing (top right)
   4 feed  (left)        5 focus   (centre)
   6 self  (bottom left) 7 charge  (bottom centre) 8 commit (bottom right)
   ```

3. Name the widgets inside them **exactly** as below. The type has to match too;
   a `TextBlock` slot given a `RichTextBlock` will not bind.

   | Slot | Name | Type | Shows |
   |---|---|---|---|
   | 1 | `AssetCaption` | TextBlock | AIRFRAME / OVERWATCH |
   | 1 | `AssetMode` | TextBlock | HOVER, FLIGHT, INBOUND, STANDBY |
   | 1 | `AssetHeadline` | TextBlock | STATION KEEPING, or a distance on foot |
   | 1 | `AssetFill` | ProgressBar | thrust, flying only |
   | 2 | `HeadingText` | TextBlock | 000–359 |
   | 2 | `DirectiveLabel` | TextBlock | DIRECTIVE / OBJECTIVE |
   | 2 | `Directive` | TextBlock | the mission's current instruction |
   | 3 | `SiteState` | TextBlock | NO CONTACT … ALERTED |
   | 3 | `CertaintyText` | TextBlock | 0–100% |
   | 3 | `CertaintyFill` | ProgressBar | the same, as a bar |
   | 4 | `FeedCaption` | TextBlock | SCAN – N RETURNS / CONTACT LOG |
   | 5 | `PromptGroup` | any Widget | shown only while there is a prompt |
   | 5 | `PromptKey` | TextBlock | F, E, or empty |
   | 5 | `Prompt` | TextBlock | TAKEDOWN, or an interactable's own text |
   | 6 | `FlightReadouts` | any Widget | shown while flying |
   | 6 | `SpeedText` | TextBlock | km/h |
   | 6 | `AltitudeText` | TextBlock | metres, or -- |
   | 6 | `VerticalSpeedText` | TextBlock | m/s, signed |
   | 6 | `GroundReadouts` | any Widget | shown on foot |
   | 6 | `IntegrityText` | TextBlock | 100% – STANDING |
   | 6 | `IntegrityFill` | ProgressBar | health |
   | 7 | `ChargeGroup` | any Widget | shown while flying |
   | 7 | `HoverFill` | ProgressBar | hover blend |
   | 7 | `ChargeHint` | TextBlock | HOLD LEFT SHIFT |
   | 8 | `CommitCaption` | TextBlock | INSERTION / EXTRACTION |
   | 8 | `CommitKey` | TextBlock | F, E, or empty |
   | 8 | `CommitHeadline` | TextBlock | DEPLOY, INBOUND, SEALED, EXTRACT … |
   | 8 | `CommitDetail` | TextBlock | the blocker, or a distance |

   Colour is set from the value's tone every frame, so leave text and bar
   colours at their defaults - a colour you set in the designer will be
   overwritten. To restyle, change `UEOHudWidget::ColourForTone`.

4. Three things a text or a bar cannot draw are left to the Blueprint, with the
   same values. Implement the **On Applied** event and read
   `Get View Model`:

   - **Feed rows** (`Feed`): a list of label, tag, distance, fill and tone.
     Rebuild a Vertical Box's children from it.
   - **Threat arcs** (`Threats`): one per guard, as a bearing relative to the
     view (-180..180), a detection 0–1 and a tone. Draw them however the design
     wants around the reticle.
   - **Compass tape**: `Heading`, plus `SiteRelativeBearing` and
     `ExtractionRelativeBearing` with their `bHas…` flags for the markers.
   - **Flight reticle**: `FlightReticleWorld` is a world point; project it with
     *Project World To Screen* and place a marker there while
     `bHasFlightReticle` is set.

5. Assign it: **Project Settings → Game → Executive Ops - HUD → Viewport
   Widget** → `WBP_HUD`. That writes `Config/DefaultGame.ini`, so commit it.

6. Run the game. The Canvas slots are gone and the widget is drawing. Anything
   blank is a slot the Blueprint has not placed yet, or one named wrongly -
   the Blueprint compiler will not complain about the latter, because the slots
   are optional, so check spelling against the table before anything else.

## The cockpit panel

The in-world panel is the second adapter over the same view model, and the
reason for building the seam. It is a `UWidgetComponent` in Cylinder mode on
the aircraft - the engine drawing a widget onto a curved surface, which is what
the old procedural-mesh panel built by hand - and it receives every frame's
view model from the HUD exactly as the viewport widget does.

Assign it in the same place: **Project Settings → Game → Executive Ops - HUD →
Panel Widget**. Usually `WBP_HUD` again, since the two bind to the same values;
a separate Blueprint is for when the canopy wants a different arrangement of
them. The panel is 84 degrees of arc at 1920×1080, so a layout authored for the
viewport carries across.

Until a Panel Widget is assigned the old panel keeps drawing the Canvas readouts
onto the glass. Assigning one is the switch: the widget panel shows in first
person and the old one stands down.
