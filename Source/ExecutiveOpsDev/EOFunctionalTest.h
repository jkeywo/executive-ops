#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "EOFunctionalTest.generated.h"

class AEOPlayerController;
class AEOOperativeCharacter;
class AEOGuardCharacter;
class AEOObjectiveTerminal;
class AEOExtractionZone;
class APawn;

/**
 * A functional test that keeps the self-test's idiom.
 *
 * UEOSelfTest was one 39-phase timed sequence: to assert anything late in it
 * you ran everything before it, an early failure took the rest with it, and
 * nothing could be run alone. Its checks were good; its container was the
 * problem. This is the container replaced with the engine's - an actor placed
 * in a map, run by itself from the Session Frontend or headless, reporting
 * through the automation framework rather than a hand-rolled log scrape.
 *
 * What it keeps: stepping on a fixed cadence with a dwell before a phase's
 * checks are sampled, because most of what needs proving is tick-driven. A
 * subclass declares its own phases and implements Step() as its own switch;
 * this class drives the cadence, counts the checks and finishes the test.
 *
 * Steps run at StepInterval. Advance() sets the next phase and how long to
 * let it settle; Wait() is the same thing without changing phase.
 */
UCLASS(Abstract)
class EXECUTIVEOPSDEV_API AEOFunctionalTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AEOFunctionalTest();

protected:
	//~ AFunctionalTest
	virtual bool IsReady_Implementation() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AFunctionalTest

	/** The subclass's sequence. Called each step once the current dwell has elapsed. */
	virtual void Step() PURE_VIRTUAL(AEOFunctionalTest::Step, );

	/** Called from Step to move on. Phase is the subclass's own enum, cast to int. */
	void Advance(int32 NextPhase, float DwellSeconds);

	/** Hold the current phase for a while longer before it is sampled again. */
	void Wait(float Seconds) { PhaseDwell = Seconds; PhaseElapsed = 0.f; }

	/** Ends the test. Result follows from the checks unless one is forced. */
	void Done();

	/**
	 * One assertion. Counted, logged in the self-test's format so the same eye
	 * reads both, and forwarded to the framework so a failure fails the test.
	 */
	bool Check(bool bCondition, const FString& What);

	int32 GetPhase() const { return Phase; }
	float GetPhaseElapsed() const { return PhaseElapsed; }

	// ---- The things every sequence needs to find -------------------------------

	AEOPlayerController* GetController() const;
	AEOOperativeCharacter* GetOperative() const;
	AEOGuardCharacter* GetGuard() const;
	AEOObjectiveTerminal* GetObjective() const;
	AEOExtractionZone* GetExtractionZone() const;

	/** The possessed pawn if it implements the aircraft interface. */
	APawn* GetAircraftPawn() const;

	/** The aircraft wherever it is, possessed or parked. */
	APawn* FindAircraftInLevel() const;

	/** Seconds between steps. Fine enough that a dwell is honoured within a frame or two. */
	static constexpr float StepInterval = 0.05f;

private:
	int32 Phase = 0;
	float PhaseDwell = 0.f;
	float PhaseElapsed = 0.f;
	float SinceStep = 0.f;
	bool bFinished = false;

	int32 Checks = 0;
	int32 Failures = 0;
};
