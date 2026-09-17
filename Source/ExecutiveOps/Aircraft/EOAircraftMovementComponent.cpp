#include "Aircraft/EOAircraftMovementComponent.h"

UEOAircraftMovementComponent::UEOAircraftMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEOAircraftMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	// Intentionally not Super. See the header: the pawn drives the move itself,
	// in order, from its own tick.
}

void UEOAircraftMovementComponent::MoveByVelocity(float DeltaSeconds)
{
	if (!UpdatedComponent || ShouldSkipUpdate(DeltaSeconds))
	{
		return;
	}

	if (Velocity.IsNearlyZero())
	{
		return;
	}

	FHitResult Hit;
	MoveUpdatedComponent(Velocity * DeltaSeconds, UpdatedComponent->GetComponentQuat(),
		/*bSweep=*/true, &Hit);

	if (!Hit.bBlockingHit)
	{
		return;
	}

	if (Hit.bStartPenetrating)
	{
		// A sweep that begins penetrating reports zero distance, and projecting
		// velocity onto the surface would delete the very component needed to get
		// out. Push clear along the escape normal instead.
		const FVector Escape = Hit.Normal * (Hit.PenetrationDepth + 1.f);
		MoveUpdatedComponent(Escape, UpdatedComponent->GetComponentQuat(), /*bSweep=*/false);
		return;
	}

	OnImpact.Broadcast(Hit);

	// Slide along the surface rather than stopping dead: clipping a building at
	// speed should cost momentum, not end the flight.
	Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal)
		* FMath::Pow(SlideRetentionPerSecond, DeltaSeconds);
}
