#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EOGuardCharacter.generated.h"

class UAnimSequence;
class UEOHealthComponent;

/**
 * The smallest alert ladder that still makes the encounter interesting.
 *
 * Deliberately not the final architecture from the GDD: no propagation, no
 * shared knowledge, no investigation of specific stimuli. Just enough states
 * that being seen is a problem the player can recover from rather than an
 * instant fail.
 */
UENUM(BlueprintType)
enum class EEOGuardState : uint8
{
	/** Walking the patrol, nothing seen. */
	Patrolling	UMETA(DisplayName = "Patrolling"),

	/** Saw something. Stops and looks, but has not committed. */
	Suspicious	UMETA(DisplayName = "Suspicious"),

	/** Confirmed. Shooting and closing. */
	Alerted		UMETA(DisplayName = "Alerted"),

	/** Lost sight. Moves to the last known position before giving up. */
	Searching	UMETA(DisplayName = "Searching"),

	Dead		UMETA(DisplayName = "Dead")
};

/**
 * One guard: walks a fixed patrol, sees the player, escalates, shoots, pursues,
 * and can lose them again.
 *
 * Steers directly toward its target rather than using navigation. On a greybox
 * route with one guard that is enough to be dangerous, and it avoids a navmesh
 * build step for something M6 will likely replace anyway.
 */
UCLASS()
class EXECUTIVEOPS_API AEOGuardCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEOGuardCharacter();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Guard")
	EEOGuardState GetGuardState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Guard")
	UEOHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Guard")
	bool IsDead() const { return State == EEOGuardState::Dead; }

	/** 0..1 toward confirmed detection. Drives the player's awareness readout. */
	UFUNCTION(BlueprintPure, Category = "Guard")
	float GetDetectionAlpha() const { return DetectionAlpha; }

	/**
	 * True if the operative is positioned for a silent kill: close, behind, and
	 * the guard has not confirmed them. An alerted guard cannot be taken down,
	 * which is what makes staying unseen worth anything.
	 */
	UFUNCTION(BlueprintPure, Category = "Guard")
	bool CanBeTakenDownBy(const AActor* Attacker) const;

	/** Kill instantly. The body stays where it falls. */
	UFUNCTION(BlueprintCallable, Category = "Guard")
	void Takedown(AActor* Attacker);

	/** Restore to the patrol start. Used by the reset command. */
	UFUNCTION(BlueprintCallable, Category = "Guard")
	void ResetGuard();

	UFUNCTION(BlueprintPure, Category = "Guard")
	bool CanSeeTarget() const { return bTargetVisible; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDied(AActor* Killer);

	/** Being shot is information. A hit from behind must not go unnoticed. */
	UFUNCTION()
	void HandleDamaged(float Amount, AActor* DamageInstigator);

	/** Line of sight plus cone plus range. */
	bool CheckVision(AActor*& OutSeen) const;

	/** True if any of the target's body line is visible from the guard's eye. */
	bool HasLineOfSightTo(const AActor& Viewed) const;

	/** Where the guard should be looking or moving: live if seen, last known if not. */
	FVector GetPursuitLocation() const;

	void UpdatePerception(float DeltaSeconds);
	void UpdateState(float DeltaSeconds);
	void UpdateMovement(float DeltaSeconds);
	void UpdateAnimation();

	void SetState(EEOGuardState NewState);
	void FireAtTarget();

	/** Steers straight at a world position. Returns true once it has arrived. */
	bool MoveToward(const FVector& Target, float DeltaSeconds, float AcceptRadius);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UEOHealthComponent> Health;

	/**
	 * Patrol waypoints, as offsets from wherever the guard is placed. Relative so
	 * a guard can be dragged around the greybox without re-authoring its route.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Patrol")
	TArray<FVector> PatrolOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Patrol")
	float PatrolSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Patrol")
	float PatrolWaitTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Patrol")
	float PursuitSpeed = 420.f;

	// ---- Perception ------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float SightRange = 2600.f;

	/** Half-angle of the vision cone, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float SightHalfAngle = 55.f;

	/** Inside this the guard notices regardless of facing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float ProximityRange = 350.f;

	/** Seconds of continuous sight before detection is confirmed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float TimeToDetect = 0.9f;

	/** Seconds of lost sight before an alerted guard drops to searching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float TimeToLose = 3.f;

	/** Eye height above the guard's origin, and the target height it looks for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float EyeHeight = 60.f;

	/** Seconds spent searching before giving up and returning to patrol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Perception")
	float SearchDuration = 6.f;

	// ---- Weapon ----------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Weapon")
	float FireInterval = 0.85f;

	/** Delay between confirming the player and the first shot: a chance to move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Weapon")
	float FirstShotDelay = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Weapon")
	float ShotDamage = 18.f;

	/** Degrees of spread. Non-zero so a moving target is genuinely harder to hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Weapon")
	float ShotSpread = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Weapon")
	float PreferredCombatRange = 900.f;

	/** How close a miss has to pass the target to be worth hearing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Guard|Combat", meta = (ClampMin = "0"))
	float NearMissRadius = 300.f;

	// ---- Takedown --------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Takedown")
	float TakedownRange = 220.f;

	/** Total arc centred on the guard's rear, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Takedown")
	float TakedownRearAngle = 110.f;

	/** Vertical separation beyond which a takedown is refused, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Takedown")
	float TakedownMaxHeightDifference = 120.f;

	// ---- Animation, assigned in the Blueprint ----------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> AimAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> FireAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> HitReactAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Animation")
	TObjectPtr<UAnimSequence> DeathAnim;

private:
	UPROPERTY(Transient)
	EEOGuardState State = EEOGuardState::Patrolling;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Target;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentAnim;

	FVector PatrolOrigin = FVector::ZeroVector;
	FRotator PatrolOriginRotation = FRotator::ZeroRotator;
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	int32 PatrolIndex = 0;
	float PatrolWaitRemaining = 0.f;

	float DetectionAlpha = 0.f;
	float TimeSinceSeen = 0.f;
	float SearchRemaining = 0.f;
	float FireCooldown = 0.f;

	bool bTargetVisible = false;
};
