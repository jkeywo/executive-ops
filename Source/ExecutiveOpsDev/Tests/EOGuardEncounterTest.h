#pragma once

#include "CoreMinimal.h"
#include "EOFunctionalTest.h"
#include "EOGuardEncounterTest.generated.h"

/**
 * One guard, every way an encounter with it can go.
 *
 * The same guard is reset between outcomes so each starts from an identical,
 * unaware state rather than inheriting the last one's mess:
 *
 *   1. the silent option - a takedown from behind, and only from behind;
 *   2. botched stealth - the guard sees the operative, fills its detection
 *      and alerts, and the silent option closes;
 *   3. the firefight - the guard shoots, and a dead operative fails the mission;
 *   4. the loud option - the operative's own weapon reaches the guard;
 *   5. breaking contact - awareness decays once the operative is gone.
 *
 * Ported from UEOSelfTest's GuardReset..GuardLostConfirm phases, check for
 * check. Placed in L_MissionTest by Scripts/place_functional_tests.py.
 */
UCLASS()
class EXECUTIVEOPSDEV_API AEOGuardEncounterTest : public AEOFunctionalTest
{
	GENERATED_BODY()

public:
	AEOGuardEncounterTest();

protected:
	virtual void Step() override;

private:
	enum class EPhase : uint8
	{
		Setup,
		Patrol,
		StealthKill,
		SeesPlayer,
		Alerted,
		ShootsPlayer,
		GunKill,
		GunAim,
		LosesPlayer,
		LostConfirm,
		Finished
	};

	EPhase Current() const { return static_cast<EPhase>(GetPhase()); }
	void Go(EPhase Next, float Dwell) { Advance(static_cast<int32>(Next), Dwell); }

	/** Values carried between phases for comparison. */
	float PatrolStartX = 0.f;
	float HealthSample = 0.f;
};
