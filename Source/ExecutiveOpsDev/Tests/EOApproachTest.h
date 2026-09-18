#pragma once

#include "CoreMinimal.h"
#include "EOFunctionalTest.h"
#include "EOApproachTest.generated.h"

/**
 * The approach, the drop, and the handover back.
 *
 * Selects the site, flies the craft there - the test stands in for the pilot,
 * cruising above the towers and descending on finals - and checks it settles
 * into a deployable hover. Then the deployment: the screen wash, the swap to
 * the operative, the locked descent, the landing, the aircraft held still
 * above. Then extraction hands the aircraft back and it still flies. Ends with
 * EOReset, so the craft is back at its start for whatever runs next.
 *
 * Ported from UEOSelfTest's MissionSelect..Extract phases, check for check.
 * Placed in L_FlightTest by Scripts/place_functional_tests.py.
 */
UCLASS()
class EXECUTIVEOPSDEV_API AEOApproachTest : public AEOFunctionalTest
{
	GENERATED_BODY()

public:
	AEOApproachTest();

protected:
	virtual void Step() override;

private:
	enum class EPhase : uint8
	{
		SelectSite,
		Navigate,
		Arrived,
		Deploy,
		Drop,
		Landed,
		Extract,
		Finished
	};

	EPhase Current() const { return static_cast<EPhase>(GetPhase()); }
	void Go(EPhase Next, float Dwell) { Advance(static_cast<int32>(Next), Dwell); }

	/**
	 * Flies the craft at the selected site: yaw to face it, throttle forward,
	 * and drop into hover on short finals so it settles instead of overshooting.
	 * Runs every step of the approach.
	 */
	void SteerTowardSite();

	/** How long the approach is given before its checks are sampled. */
	static constexpr float ApproachSeconds = 32.f;

	/** How long Arrived will hold for the arena to stream in before it checks. */
	static constexpr float ArenaWaitLimit = 15.f;
	static constexpr float ArenaWaitStep = 0.25f;

	float DistanceSample = 0.f;
	float AltitudeSample = 0.f;
	float ArenaWaitSeconds = 0.f;
};
