# The arena streams into the flight map

The ground route used to exist only as `L_MissionTest`, a map of its own that
nothing ever loaded from the flight map. Deploying threw the operative onto a
bare roof. The GDD wants the drop to read as one continuous game, and the
ground to be a piece of the city rather than a separate level.

Resolved by streaming, not travelling. `AEOMissionSite` holds a soft reference
to `L_SiteArena` and loads it as a runtime level instance
(`ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr`) with the arena's
origin on the site's insertion point. The player controller asks for it when the
aircraft enters the hover volume and lets it go again beyond twice the hover
radius, unless the mission has moved past `InFlight`. Deployment is gated on the
level being visible, through the same blocker string the HUD reads.

Consequences and choices worth keeping:

- **ADR 0002 stands.** One world carries the whole mission, so mission state
  stays a `UWorldSubsystem`. Nothing here has to outlive a world.
- **One streaming object, toggled.** A stream-out followed by a stream-in gives
  a fresh copy of the level either way, so the site keeps one handle and flips
  `ShouldBeLoaded`/`ShouldBeVisible` rather than removing and recreating it.
  Every stream-out is followed by a forced garbage collection, which is why the
  unload radius is wider than the load radius.
- **Location only.** The level transform carries no rotation. A rotated arena
  would rotate the guard's fixed-axis patrol offsets and flag texture streaming,
  and the site is placed axis-aligned anyway.
- **The route is shared.** `Scripts/eo_route.py` places the same blocks into
  `L_MissionTest` (with lighting and a PlayerStart, for the ground tests) and
  `L_SiteArena` (with neither, so nothing doubles up against the flight map).
- **The deck sits two centimetres proud of the roof.** Coplanar faces flicker
  and give the landing two surfaces; the arena's floor is authored at z=-48.
- **Greybox, accepted:** the 120 m deck overhangs the 26 m mission building, two
  neighbouring towers pierce it off to the side of the route, and the far end of
  the route already hangs past the deck in `L_MissionTest`. On the roof that
  last one is a fall to street level rather than the kill volume.
- **Not cooked.** A soft-referenced map is not packaged automatically; add
  `L_SiteArena` to Maps To Cook when packaging ever matters.
