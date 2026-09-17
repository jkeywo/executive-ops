# Deepening log

Running record of the eight-candidate architecture pass on the
`architecture-deepening` branch. Decisions taken without the user present are
marked **[autonomous]** and are the ones worth reviewing first.

Directions were settled by grilling before implementation began; the reasoning
behind each lives in `Docs/adr/`. This file records what actually happened,
including the places where the agreed direction met something the codebase did
not support.

## Baseline

Editor target builds clean (27s incremental). Both self-test suites run from
`Scripts/run_selftests.ps1`, and the suite result is the gate for every commit
below.

## Candidates

### C6 - Load the feedback presets off the hot path  *(done)*

Async warm at `Initialize`, events skip silently until their asset lands, and
event identity moves from `FName` to native gameplay tags.

- **Correction to the review.** The review said GameplayTags "is not enabled in
  this project at all", implying a plugin. It is an engine *module*, not a
  plugin, so adopting it cost one line in `ExecutiveOps.Build.cs` and no
  `.uproject` change. GameplayAbilities is the plugin; that is still not used.
- **[autonomous]** Call sites were left untouched. `FNativeGameplayTag` converts
  implicitly to `FGameplayTag`, so all 33 `EOFeedbackEvents::X` uses compile
  unchanged and the diff stays in the feedback module rather than spreading
  across five gameplay files.
- **[autonomous]** Tag strings are `Feedback.<Category>.<Event>`, derived from the
  existing single-underscore names. The Python generator keeps the readable
  `Pistol_Fire` keys and converts at the point of writing, so the generator still
  diffs against `EOFeedbackEvents.h` one symbol per line.
- **[autonomous]** `unreal.GameplayTag(tag_name=...)` and `set_editor_property`
  both fail - `TagName` is read-only from Python. `import_text` is the only route
  the struct exposes, and is what the generator uses.
- Verified by reading the regenerated asset back: 34 presets, every key a
  `Feedback.*` tag, none left as a bare name.

