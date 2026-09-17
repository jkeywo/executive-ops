#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Character/EOTraversalComponent.h"
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

	/**
	 * True if -EOGroundTest was passed: runs the M4 traversal checks against the
	 * ground route instead of the flight sequence, since the two live in
	 * different maps.
	 */
	static bool IsGroundTest();

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

		// -EOGroundTest path.
		GroundSetup,
		VaultCheck,
		MantleCheck,
		ClimbCheck,
		StowInterrupt,
		SlideCheck,
		SlideConfirm,

		// M5: the same encounter, five different outcomes.
		GuardReset,
		GuardPatrol,
		GuardStealthKill,
		GuardSeesPlayer,
		GuardAlerted,
		GuardShootsPlayer,
		GuardGunKill,
		GuardGunAim,
		GuardGunFire,
		GuardLosesPlayer,
		GuardLostConfirm,

		// M6: the whole ground mission, run twice to prove it repeats.
		MissionSetup,
		MissionObjective,
		MissionExtract,
		MissionPickup,
		MissionSecondRun,

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

	/**
	 * Places the operative a stride in front of an obstacle facing it, then
	 * reports what the traversal scan makes of it. Teleporting rather than
	 * running there keeps each verb an isolated, repeatable check.
	 */
	bool CheckTraversalAt(const FVector& StandLocation, float FacingYaw,
		EEOTraversalType Expected, const TCHAR* Label);

	/** The possessed pawn as an operative, or null. */
	class AEOOperativeCharacter* GetOperative() const;

	/** The single guard in the level, or null. */
	class AEOGuardCharacter* GetGuard() const;

	/** The aircraft in the level whether or not it is the possessed pawn. */
	APawn* FindAircraftInLevel() const;

	/** Puts the operative and the guard back to a known, unaware starting state. */
	void ResetEncounter();

	/** Drives the mission to OnGround, standing in for a completed insertion. */
	bool EnterGroundMission();

	/** Walks one full mission: objective then extraction. Returns true if it closed. */
	void RunMissionLeg(int32 Attempt);

	class AEOObjectiveTerminal* GetObjective() const;
	class AEOExtractionZone* GetExtractionZone() const;

	/** Which pass through the mission is running: 1 first, 2 the repeat. */
	int32 MissionAttempt = 0;

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
