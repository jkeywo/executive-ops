#include "Character/EOOperativeAnimInstance.h"

#include "Character/EOOperativeCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UEOOperativeAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Resolved once, on the game thread. The worker-thread update below must not
	// go looking for components; it reads through these.
	Operative = Cast<AEOOperativeCharacter>(TryGetPawnOwner());
	Movement = Operative ? Operative->GetCharacterMovement() : nullptr;

	if (Operative)
	{
		PreviousYaw = Operative->GetActorRotation().Yaw;
	}
}

void UEOOperativeAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!Operative || !Movement || DeltaSeconds <= 0.f)
	{
		return;
	}

	// ---- Locomotion ------------------------------------------------------------

	const FVector Velocity = Movement->Velocity;
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
	const FRotator Facing = Operative->GetActorRotation();

	GroundSpeed = Velocity2D.Size();
	bMoving = GroundSpeed > MovingSpeedThreshold;
	bAccelerating = !Movement->GetCurrentAcceleration().IsNearlyZero();

	// Where the body is going relative to where it is pointing. Zero when
	// running forward, +-90 strafing, +-180 backpedalling.
	MoveDirection = bMoving
		? FRotator::NormalizeAxis(Velocity2D.Rotation().Yaw - Facing.Yaw)
		: 0.f;

	// The blend space's X axis leans into a turn. Derived from yaw rate, which is
	// what the pack's own character supplied, and eased so a mouse flick does not
	// snap the torso.
	const float YawRate = FRotator::NormalizeAxis(Facing.Yaw - PreviousYaw) / DeltaSeconds;
	PreviousYaw = Facing.Yaw;

	const float TargetLean = bMoving
		? FMath::Clamp(YawRate / FullLeanTurnRate, -1.f, 1.f)
		: 0.f;
	LeanAmount = FMath::FInterpTo(LeanAmount, TargetLean, DeltaSeconds, LeanInterpSpeed);

	// The speed the operative was doing when input let go decides which stop
	// clip plays. Held while still accelerating, frozen the moment they are not.
	if (bAccelerating)
	{
		StoppingSpeed = GroundSpeed;
	}

	// ---- Character state -------------------------------------------------------

	bIsAiming = Operative->IsAiming();
	bIsSprinting = Operative->IsSprinting();
	bIsSliding = Operative->IsSliding();

	// ---- Falling ---------------------------------------------------------------

	const bool bWasInAir = bInAir;
	bInAir = Movement->IsFalling();

	const float CurrentZ = Operative->GetActorLocation().Z;

	if (bInAir && !bWasInAir)
	{
		// Left the ground this frame: start the drop from here.
		FallStartZ = CurrentZ;
		TimeFalling = 0.f;
	}

	if (bInAir)
	{
		TimeFalling += DeltaSeconds;

		// Only the descent counts. A jump rises first, and that rise must not be
		// mistaken for distance fallen.
		FallHeight = FMath::Max(0.f, FallStartZ - CurrentZ);
		FallStartZ = FMath::Max(FallStartZ, CurrentZ);
	}
	else
	{
		FallHeight = 0.f;
		TimeFalling = 0.f;
	}

	bIsLongFall = bInAir && (FallHeight > LongFallHeight || TimeFalling > LongFallTime);
}
