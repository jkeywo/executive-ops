#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EOSelfTest.generated.h"

class AEOPlayerController;

/**
 * Automated check that each milestone's "done when" actually holds.
 *
 * Runs as a sequence of timed phases rather than a single function, because most
 * of what needs proving is tick-driven: a craft accelerating, momentum bleeding
 * off, a mode blending. Each phase drives the game through the public interfaces
 * and then samples the result a fixed time later.
 *
 * Launch with -EOSelfTest to run it, and -EOSelfTestExit to quit afterwards with
 * an exit code of 0 only if every check passed.
 */
UCLASS()
class EXECUTIVEOPS_API UEOSelfTest : public UObject
{
	GENERATED_BODY()

public:
	/** True if -EOSelfTest was passed on the command line. */
	static bool IsRequested();

	/** True if -EOSelfTestExit was passed on the command line. */
	static bool ShouldExitAfterRun();

	/** Begins the phase sequence. Results are logged as they are produced. */
	void Start(AEOPlayerController* InController);

private:
	/** Phases run in declaration order; each one advances when its dwell elapses. */
	enum class EPhase : uint8
	{
		CoreState,
		DeployGuard,
		Reset,
		IllegalTransitions,
		FlightAccelerate,
		FlightCoast,
		HoverEngage,
		HoverSpeedClamp,
		MissionSelect,
		NavigateToSite,
		ArrivedAtSite,
		DeploymentAssist,
		DeploymentDrop,
		DeploymentLanded,
		Extract,
		Done
	};

	void Step();
	void Advance(EPhase Next, float DwellSeconds);

	/**
	 * Flies the craft at the selected site: yaw to face it, throttle forward, and
	 * drop into hover on short finals so it settles instead of overshooting.
	 * Runs every step during the approach rather than only at the dwell.
	 */
	void SteerTowardSite();

	void Check(bool bCondition, const FString& Description);
	void Finish();

	/** Returns the possessed pawn only if it implements the aircraft interface. */
	APawn* GetAircraftPawn() const;

	UPROPERTY(Transient)
	TObjectPtr<AEOPlayerController> Controller;

	FTimerHandle StepTimer;

	EPhase Phase = EPhase::CoreState;
	float PhaseDwell = 0.f;
	float PhaseElapsed = 0.f;

	int32 Failures = 0;
	int32 Checks = 0;

	/** Values carried between phases for comparison. */
	float SpeedSample = 0.f;
	float DistanceSample = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<APawn> BootPawn;
};
