# Feedback events are identified by GameplayTags

Feedback event identity moves from `extern const FName` constants to GameplayTags,
enabling the GameplayTags plugin for the first time.

FName constants already gave compile-time-checked symbols and kept preset authoring
independent, so this buys hierarchy and matching rather than safety:
`Feedback.Weapon.Pistol.Fire` can be matched as a family, and the editor
autocompletes it.

The wider reason is that identity was about to be invented twice. Once the plugin
is enabled it is the natural key for anything else that needs one — AI stimuli,
mission conditions, damage classification — rather than each system growing its
own registry.
