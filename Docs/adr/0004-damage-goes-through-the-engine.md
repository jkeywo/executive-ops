# Damage goes through Unreal's damage pipeline

Damage was originally a direct call to `UEOHealthComponent::ApplyDamage`, bypassing
`TakeDamage`, `UGameplayStatics::ApplyDamage` and `UDamageType` entirely. Shots now
go through `ApplyPointDamage`; the health component subscribes to its owner's damage
delegate.

The bespoke path was clean and genuinely class-agnostic, so this is a deliberate
trade: engine plumbing for damage types, instigator and causer, hit location and a
Blueprint-visible seam.

The one subtlety worth preserving: a takedown must not read as "I was shot", or the
guard's reaction to being fired on triggers on a silent kill. That distinction used
to be the difference between calling `Kill()` and calling `ApplyDamage()`. It is now
carried by `UEOTakedownDamageType`, so the difference is data on the damage event
rather than a choice of function.
