# Asset-authored content replaces C++-authored content

The project originally kept the HUD, the input mappings and the guard's state
machine in C++ specifically so the repo stayed free of binary assets and every
change could be read in a diff. That constraint is now lifted: the HUD moves to
UMG, input mappings move to Input Mapping Context assets, and the guard's brain
moves to a StateTree.

The reasoning was sound while the project was a greybox with no art pass, and the
comments defending it in `EOPlayerHUD.h`, `EONavigationHUD.h` and `EOInputConfig.h`
should be read as history, not as current policy. What changed is that each of
those C++-authored systems had started to cost more than it saved: Canvas cannot
do the layout the HUD now needs, the input config's invert settings were provably
dead because the object was rebuilt every run, and the guard's five-state enum is
about to grow perception and alert propagation.

Where a script can generate the asset — as `m0_setup.py` already does for the
Blueprints and `m8_build_feedback_presets.py` does for the preset set — the script
stays the source of truth and the asset is its output. That preserves most of the
diff-readability the original decision was protecting.
