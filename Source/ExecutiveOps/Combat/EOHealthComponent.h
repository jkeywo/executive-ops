#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEOHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEODied, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEODamaged, float, Amount, AActor*, DamageInstigator, const UDamageType*, DamageType);

/**
 * The simple health model M5 asks for, shared by the operative and the guard.
 *
 * No armour, no regeneration. Both sides die quickly, which is the point: the
 * operative wins through speed and surprise, not durability, and a firefight
 * should read as a mistake rather than a fight to be won.
 *
 * Damage arrives through Unreal's own pipeline - the component binds its owner's
 * OnTakeAnyDamage - so anything that can call ApplyDamage can hurt anything with
 * this component, without either side knowing about the other. The damage type
 * comes along with it, which is how a takedown stays distinguishable from a shot
 * now that both take the same route. See Docs/adr/0004.
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

	/**
	 * Applies damage directly, bypassing the engine's pipeline.
	 *
	 * Kept for the reset command and the tests, which need a deterministic poke
	 * that does not depend on a damage event being routed. Gameplay should go
	 * through UGameplayStatics::ApplyPointDamage instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(float Amount, AActor* Instigator, const UDamageType* DamageType = nullptr);

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

	/** Bound to the owner's OnTakeAnyDamage. The only route gameplay damage takes. */
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

private:
	UPROPERTY(Transient)
	float Health = 0.f;

	bool bDead = false;
};
