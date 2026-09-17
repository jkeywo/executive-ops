# Look inversion is read per event, not baked into the mapping

`bInvertMouseY` and `bInvertStickY` were `EditAnywhere` properties on
`UEOInputConfig`, and could never take effect. The config is built with
`NewObject` on every run, the mappings bake the inversion in once behind a
`bBuilt` guard, and there is no rebuild path - so the object the editor let you
change was discarded before it was ever consulted.

They now live on `UEOInputSettings`, a `UDeveloperSettings`, and are read at the
moment a look value is consumed. Changing one applies immediately, with no
context rebuild and no second source of truth.

These are project settings rather than per-player ones. The grilling chose
Enhanced Input's own user settings, which persist per user and are the right home
for a player-facing options screen. That is a larger job - a settings subclass,
`bEnableUserSettings`, and save-game plumbing - and it is not what was broken
here. Worth doing when there is an options screen to hang it off; until then this
is the smallest change that makes the setting real.
