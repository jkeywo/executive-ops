#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EOOperativeAnimInstance.generated.h"

class AEOOperativeCharacter;
class UCharacterMovementComponent;

/**
 * What the locomotion graph needs to know, computed in C++.
 *
 * The graph is the Dynamic Locomotion pack's, and it originally read its inputs
 * by casting the pawn to the pack's own demo character. That cast can never
 * succeed here, so the values it fed - speed, lean, the leap threshold - sat
 * frozen at zero, which is why the operative never ran and every jump read as a
 * dive.
 *
 * Property names deliberately avoid colliding with the graph's existing
 * Blueprint variables (Speed, LateralSpeed, IsInAir?, ...). The event graph sets
 * those from these, which keeps the state machine's transition rules untouched -
 * nine of them reference those variables by name, and rebinding each one by hand
 * is exactly the kind of fragile step worth designing out.
 */
UCLASS()
class EXECUTIVEOPS_API UEOOperativeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	// ---- Locomotion ------------------------------------------------------------

	/** Ground speed in cm/s. Drives the blend space, whose samples are authored at 180, 500 and 950. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	/** Travel direction relative to facing, -180..180. For the aiming strafe set. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float MoveDirection = 0.f;

	/**
	 * Lean, -1..1, from how fast the operative is turning.
	 *
	 * The blend space's X axis is a lean, not a sideways speed - the pack drove
	 * it from its character's turn rate, and feeding raw cm/s there would peg it
	 * to one edge permanently.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LeanAmount = 0.f;

	/** Speed carried into a stop, which decides which foot the stop lands on. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float StoppingSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bAccelerating = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bInAir = false;

	// ---- Character state -------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsSliding = false;

	// ---- Falling ---------------------------------------------------------------

	/** How far the operative has dropped since leaving the ground, in centimetres. */
	UPROPERTY(BlueprintReadOnly, Category = "Falling")
	float FallHeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Falling")
	float TimeFalling = 0.f;

	/**
	 * True once a fall has gone on long enough to stop reading as a jump.
	 *
	 * This is the distinction the pack's graph could not make: it chose between
	 * the controlled hop and the committed dive once, on leaving the ground, from
	 * horizontal speed alone. A jump should look like a jump until the ground
	 * fails to arrive.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Falling")
	bool bIsLongFall = false;

	/** Drop beyond which a fall stops being a jump. */
	UPROPERTY(EditDefaultsOnly, Category = "Falling", meta = (ClampMin = "0"))
	float LongFallHeight = 350.f;

	/** Or time in the air, for a fall that has barely descended yet. */
	UPROPERTY(EditDefaultsOnly, Category = "Falling", meta = (ClampMin = "0"))
	float LongFallTime = 0.6f;

	// ---- Thresholds the graph used to read off the pack's character ------------

	/**
	 * Horizontal speed at which leaving the ground is a running leap, not a jump.
	 *
	 * Above the run speed on purpose: leaving the ground always looks like a
	 * jump, and the dive is reached through bIsLongFall instead of on takeoff.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0"))
	float LeapSpeedThreshold = 1200.f;

	/** Below this the operative counts as stationary. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0"))
	float MovingSpeedThreshold = 10.f;

	/** Degrees per second of turn that reads as a full lean. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "1"))
	float FullLeanTurnRate = 120.f;

	/** How quickly the lean eases, so a flick of the mouse does not snap the body. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.01"))
	float LeanInterpSpeed = 6.f;

private:
	/** Cached on the game thread; only read on the worker thread. */
	UPROPERTY(Transient)
	TObjectPtr<AEOOperativeCharacter> Operative = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> Movement = nullptr;

	/** Actor Z at the moment the ground was left, for measuring the drop. */
	float FallStartZ = 0.f;

	float PreviousYaw = 0.f;
};
