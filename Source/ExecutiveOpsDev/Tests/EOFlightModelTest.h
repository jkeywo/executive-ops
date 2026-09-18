#pragma once

#include "CoreMinimal.h"
#include "EOFunctionalTest.h"
#include "EOFlightModelTest.generated.h"

/**
 * The game boots into a controllable aircraft, and the aircraft flies.
 *
 * Boot state and the mission's gates first - deployment refused away from the
 * site, illegal transitions refused - then the flight model: it accelerates
 * under input, coasts when released, sheds speed into a hover and never
 * exceeds the hover cap, and the cockpit panel follows the view. Ends with
 * EOReset, so the craft is back at its start for whatever runs next.
 *
 * Ported from UEOSelfTest's CoreState..HoverSpeedClamp phases, check for
 * check. Placed in L_FlightTest by Scripts/place_functional_tests.py.
 */
UCLASS()
class EXECUTIVEOPSDEV_API AEOFlightModelTest : public AEOFunctionalTest
{
	GENERATED_BODY()

public:
	AEOFlightModelTest();

protected:
	virtual void Step() override;

private:
	enum class EPhase : uint8
	{
		BootState,
		DeployGate,
		Reset,
		IllegalTransitions,
		Accelerate,
		Coast,
		HoverEngage,
		HoverSpeedClamp,
		Finished
	};

	EPhase Current() const { return static_cast<EPhase>(GetPhase()); }
	void Go(EPhase Next, float Dwell) { Advance(static_cast<int32>(Next), Dwell); }

	float SpeedSample = 0.f;
};
