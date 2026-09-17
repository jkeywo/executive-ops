#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "EOAircraftMovementComponent.generated.h"

/**
 * Moves the craft, and nothing else.
 *
 * The pawn decides what the craft is trying to do - throttle, hover, a scripted
 * approach - and writes the result into Velocity. This component is the single
 * place that turns a velocity into a position: sweep, escape a penetration,
 * slide along whatever was hit.
 *
 * That block used to exist twice, once in the piloted path and once in the
 * scripted arrival, which meant a fix to one was a fix to neither. It is also
 * why the craft had no movement component at all and GetMovementComponent()
 * returned null, so nothing in the engine could see it move.
 *
 * Kinematic on purpose: swept transforms rather than simulated forces. A flight
 * model you author directly is far easier to tune for feel than one mediated by
 * a solver, and the scripted arrival stays deterministic, which the self-tests
 * depend on. See Docs/adr/0003.
 */
UCLASS()
class EXECUTIVEOPS_API UEOAircraftMovementComponent : public UFloatingPawnMovement
{
	GENERATED_BODY()

public:
	UEOAircraftMovementComponent();

	/**
	 * How much speed survives a graze, per second of contact.
	 *
	 * Damped per unit time rather than per frame: a per-frame halving stalls the
	 * craft completely at high refresh rates.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Movement",
		meta = (ClampMin = "0", ClampMax = "1"))
	float SlideRetentionPerSecond = 0.15f;

	/**
	 * Moves by the current Velocity and resolves whatever it hits.
	 *
	 * Called by the pawn once it has decided this frame's velocity, rather than
	 * from this component's own tick, because the decision and the move have to
	 * happen in that order within one frame.
	 */
	void MoveByVelocity(float DeltaSeconds);

	/** Broadcast on a blocking hit, so the pawn can turn it into feedback. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FEOAircraftImpact, const FHitResult&);
	FEOAircraftImpact OnImpact;

protected:
	/**
	 * Deliberately does not call Super.
	 *
	 * UFloatingPawnMovement's tick applies control input to velocity and then
	 * moves, which would fight the pawn's own flight model and move the craft a
	 * second time in the same frame. What this class wants from the base is its
	 * velocity plumbing and MaxSpeed/Acceleration/Deceleration, not its
	 * integration.
	 */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
};
