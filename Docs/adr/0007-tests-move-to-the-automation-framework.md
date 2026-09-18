# Tests move to the automation framework, in their own module

The 143 checks in `UEOSelfTest` move onto Unreal's own test seams: automation tests
for anything that needs no world, `AFunctionalTest` actors for the sequences that
do. The bespoke phase machine, the custom command-line switches, the hand-rolled
`Check()` and the log-scraping PowerShell runner all go.

The two end-to-end sequences survive as functional tests — they catch integration
failures no module-level test will, and they are the reason the framework is
trustworthy today.

Two things this fixes beyond ergonomics. Test and cheat code moves to a module the
shipping Game target does not link, rather than compiling into every configuration
as it does now. And checks assert on behaviour and on relationships to the module's
own tuning values, not on literal magnitudes copied from those values — so a
retune during a feel pass no longer breaks the tests that are supposed to be
guarding it.

**Update.** The module split is done: `ExecutiveOpsDev`, marked `DeveloperTool`,
holds the self-test and the cheats, and is excluded from Shipping while remaining
available in Development Game builds and the editor.

The dependency inverts through two engine hooks rather than anything bespoke:
`UCheatManager::RegisterForOnCheatManagerCreated` for the cheats, which become a
`UCheatManagerExtension` so no controller names a `CheatClass`, and
`FGameModeEvents::OnGameModePostLoginEvent` for the harness, which now starts
itself. The game module refers to neither.
