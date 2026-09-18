#pragma once

#include "CoreMinimal.h"
#include "EOFunctionalTest.h"
#include "EOMissionLoopTest.generated.h"

/**
 * The whole ground mission, run twice.
 *
 * Insertion stands in for the flight and the drop, which the flight sequence
 * covers; from there it is the real thing: reach the terminal, complete the
 * objective, cross back to the pad, call the extraction, wait for the craft to
 * actually fly in, get handed the aircraft. Then EOReset, and the same again -
 * because the point of M6 is that it has to be playable again without debug
 * help, and a mission that only works once is not a loop.
 *
 * Ported from UEOSelfTest's MissionSetup..MissionSecondRun phases, check for
 * check. Placed in L_MissionTest by Scripts/place_functional_tests.py; runs on
 * its own as Project.Functional Tests.L_MissionTest.MissionLoop.
 */
UCLASS()
class EXECUTIVEOPSDEV_API AEOMissionLoopTest : public AEOFunctionalTest
{
	GENERATED_BODY()

public:
	AEOMissionLoopTest();

protected:
	virtual void Step() override;

private:
	enum class EPhase : uint8
	{
		Setup,
		Objective,
		Extract,
		Pickup,
		SecondRun,
		Finished
	};

	EPhase Current() const { return static_cast<EPhase>(GetPhase()); }
	void Go(EPhase Next, float Dwell) { Advance(static_cast<int32>(Next), Dwell); }

	FString Pass() const { return FString::Printf(TEXT("run %d:"), Attempt); }

	int32 Attempt = 1;
	float DistanceSample = 0.f;
};
