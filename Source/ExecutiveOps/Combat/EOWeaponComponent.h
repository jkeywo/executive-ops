#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EOWeaponComponent.generated.h"

/**
 * What a shot did, for the caller that has to react to it.
 *
 * The component fires and deals damage; deciding what a hit or a miss means -
 * an impact preset, a whizz past the player's ear, a reaction - belongs to
 * whoever pulled the trigger, and needs these facts to do it.
 */
USTRUCT(BlueprintType)
struct FEOShotResult
{
	GENERATED_BODY()

	/** False when the weapon refused: still cooling down. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bFired = false;

	/** The direction actually fired, after spread. Not the direction asked for. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FHitResult Hit;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bBlockingHit = false;

	/** True when the thing hit can take damage, which is a different question. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bHitDamageable = false;
};

/**
 * One shot, behind one call.
 *
 * The operative and the guard had the same weapon written twice: the same
 * ECC_Pawn trace with the same reasoning in the same words, the same spread
 * cone, the same damage. Only the aiming differed, and that is the part that
 * genuinely does differ - the operative aims down a camera the guard does not
 * have.
 *
 * So aiming stays with the caller and everything behind it lives here. The
 * caller says where the muzzle is and what it is pointing at; this decides
 * whether the weapon is ready, applies the spread, traces, deals the damage and
 * reports what happened.
 *
 * It also gives the shot an interface of its own to be tested through. The
 * pistol check was intermittent across three commits because the only way to
 * observe a shot was to fly the whole game to a guard and take one.
 */
UCLASS(ClassGroup = (ExecutiveOps), meta = (BlueprintSpawnableComponent))
class EXECUTIVEOPS_API UEOWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOWeaponComponent();

	/**
	 * Fires from Muzzle toward AimPoint, if the weapon is ready.
	 *
	 * bAccurate picks the tight cone over the hip one - it is the only reason to
	 * aim. The caller passes an aim point rather than a direction so that the
	 * spread is always applied to the real shot line, which is what the flaky
	 * pistol check turned out to hinge on.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FEOShotResult TryFire(const FVector& Muzzle, const FVector& AimPoint, bool bAccurate = false);

	/** Seconds until the weapon will fire again. Zero when ready. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetCooldownRemaining() const { return Cooldown; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReady() const { return Cooldown <= 0.f; }

	/** Clears the cooldown. For the reset command and the tests. */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ResetWeapon() { Cooldown = 0.f; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float Damage = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float Range = 8000.f;

	/** Minimum seconds between shots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float Interval = 0.28f;

	/** Hip spread in degrees. Aiming removes it, which is the only reason to aim. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float HipSpread = 4.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float AimSpread = 0.4f;

	/**
	 * Fired at the muzzle on every shot. The guard's report is not the pistol's.
	 *
	 * Impact feedback deliberately is not here: what an impact means depends on
	 * what was hit and who is watching, so callers decide it from the result.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Feedback")
	FGameplayTag FireEvent;

	/** Component the muzzle effect rides, when there is one. */
	UPROPERTY(BlueprintReadWrite, Category = "Weapon|Feedback")
	TObjectPtr<USceneComponent> MuzzleAttachment;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	float Cooldown = 0.f;
};
