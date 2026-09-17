#include "Combat/EOHealthComponent.h"

#include "ExecutiveOps.h"

UEOHealthComponent::UEOHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEOHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

float UEOHealthComponent::GetHealthFraction() const
{
	return MaxHealth > KINDA_SMALL_NUMBER ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
}

float UEOHealthComponent::ApplyDamage(float Amount, AActor* Instigator)
{
	// Already dead, or a heal dressed up as damage: neither should reach the
	// death path or re-broadcast.
	if (bDead || Amount <= 0.f)
	{
		return 0.f;
	}

	const float Applied = FMath::Min(Amount, Health);
	Health -= Applied;

	OnHealthChanged.Broadcast(Health, -Applied);
	OnDamaged.Broadcast(Applied, Instigator);

	UE_LOG(LogExecutiveOps, Verbose, TEXT("%s took %.0f damage from %s (%.0f left)"),
		*GetNameSafe(GetOwner()), Applied, *GetNameSafe(Instigator), Health);

	if (Health <= 0.f)
	{
		bDead = true;
		OnDied.Broadcast(Instigator);
	}

	return Applied;
}

void UEOHealthComponent::Kill(AActor* Instigator)
{
	if (bDead)
	{
		return;
	}

	const float Remaining = Health;
	Health = 0.f;
	bDead = true;

	OnHealthChanged.Broadcast(Health, -Remaining);
	OnDied.Broadcast(Instigator);
}

void UEOHealthComponent::Revive()
{
	bDead = false;
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);
}
