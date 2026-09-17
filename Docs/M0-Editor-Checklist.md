# M0 — Editor Checklist

Everything in M0 that can live in source control as text is already committed. These are the
remaining steps, which require the editor because their outputs are binary `.uasset`/`.umap`
files. Each should take minutes, not hours — M0 is scaffolding, not content.

## 1. Maps

Create two empty levels under `Content/Maps/`:

| Map | Purpose |
|---|---|
| `L_FlightTest` | Empty flight map. M1 fills it with the greybox flight playground. |
| `L_MissionTest` | Empty mission map. M4/M6 fill it with the ground route and arena. |

For each:

1. **File → New Level → Empty Open World** (`L_FlightTest`, so World Partition is in place
   from the start per GDD §49.1) or **Empty Level** (`L_MissionTest` — a fixed arena does not
   need partitioning).
2. Add a `Directional Light`, `Sky Atmosphere`, `Sky Light` and `Player Start` so the map is
   visible and boots.
3. Save into `Content/Maps/`.

`Config/DefaultEngine.ini` already points `GameDefaultMap` and `EditorStartupMap` at
`/Game/Maps/L_FlightTest`, so this starts working as soon as the map exists.

## 2. Blueprint subclasses

Create under `Content/Blueprints/`:

| Blueprint | Parent | Why a Blueprint |
|---|---|---|
| `BP_Aircraft` | `EOAircraftPawn` | Assign a greybox hull static mesh, position `DeploymentSocket` |
| `BP_Operative` | `EOOperativeCharacter` | Assign the character rig's skeletal mesh + anim blueprint |
| `BP_GameMode` | `EOGameModeBase` | Point `DefaultPawnClass` at `BP_Operative` |
| `BP_PlayerController` | `EOPlayerController` | Point `AircraftClass`/`OperativeClass` at the two BPs above |

Then set `BP_GameMode` as the **Project Settings → Maps & Modes → Default GameMode**, and set
its Player Controller Class to `BP_PlayerController`.

The C++ classes work without any of this — the Blueprints exist only to hold asset references,
which is the one thing C++ should not hardcode.

## 3. Character rig

1. Import the chosen production character rig into `Content/Characters/Operative/`.
2. Assign its skeletal mesh to `BP_Operative`'s `Mesh` component.
3. Rotate the mesh −90° on yaw and offset Z by −96 so it sits in the capsule.
4. Assign the rig's animation blueprint.

## 4. Animation packs

Import the owned packs into `Content/Animations/`:

- **Open World** set — vault, climb, fall, jump, crawl, slide, cover, armed locomotion.
- **Fighting** set — unarmed movement and combat.

Retarget both to the operative's skeleton. Do **not** build a traversal system yet — M4 owns
that. M0 only needs the animations present and retargeted so M4 starts from real data rather
than placeholders.

## 5. Verify the M0 "done when"

> The project reliably boots into a test map and the player can possess either the operative
> or a placeholder aircraft.

1. PIE into `L_FlightTest`. The debug HUD shows `Control: Aircraft`, `Mission: Inactive`.
2. `WASD` / `Space` / `Ctrl` / `Q` / `E` move the aircraft; the speed readout responds.
3. Hold `Left Shift` and slow down — `Deploy:` turns green and reads `READY [F]`.
4. Press `F`. Control switches to the operative; `Mission:` reads `On Ground`.
5. `WASD` / mouse / `Space` / `Left Shift` drive the operative.
6. Console `EOExtract` returns control to the aircraft.
7. Console `EOReset` puts both pawns back and `Mission:` returns to `Inactive`.
8. Console `EOPossessAircraft` / `EOPossessOperative` switch freely.

If all eight pass, M0 is done.
