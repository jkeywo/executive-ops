#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEOHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEODied, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEODamaged, float, Amount, AActor*, DamageInstigator);

/**
 * The simple health model M5 asks for, shared by the operative and the guard.
 *
 * No armour, no damage types, no regeneration. Both sides die quickly, which is
 * the point: the operative wins through speed and surprise, not durability, and
 * a firefight should read as a mistake rather than a fight to be won.
 */
UCLASS(ClassGroup = (ExecutiveOps), meta = (BlueprintSpawnableComponent))
class EXECUTIVEOPS_API UEOHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOHealthComponent();

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FEOHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FEODied OnDied;

	/** Carries the instigator, which OnHealthChanged does not. */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FEODamaged OnDamaged;

	/** Returns the damage actually applied. Zero if already dead. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(float Amount, AActor* Instigator);

	/** Kill outright, whatever the health. Used by the takedown. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill(AActor* Instigator);

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthFraction() const;

	/** Back to full, alive. Used by the reset command. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Revive();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

private:
	UPROPERTY(Transient)
	float Health = 0.f;

	bool bDead = false;
};
