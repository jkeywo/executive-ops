#pragma once

#include "CoreMinimal.h"
#include "EOFunctionalTest.h"
#include "EOTraversalTest.generated.h"

enum class EEOTraversalType : uint8;

/**
 * Ground movement: the traversal verbs, the slide, and the mesh staying on
 * the capsule.
 *
 * Each verb is checked in isolation against the obstacle built for it in
 * L_MissionTest - the 90cm wall, the 180cm ledge, the 380cm wall - by
 * teleporting the operative a stride in front of it, reading what the scan
 * makes of it, starting the traversal and, a dwell later, checking where it
 * ended and that collision came back. Then the negatives: open ground reports
 * nothing, and stowing mid-traversal leaves collision off rather than on.
 *
 * Ported from UEOSelfTest's GroundSetup..MeshDriftRun phases, check for check.
 * Placed in L_MissionTest by Scripts/place_functional_tests.py.
 */
UCLASS()
class EXECUTIVEOPSDEV_API AEOTraversalTest : public AEOFunctionalTest
{
	GENERATED_BODY()

public:
	AEOTraversalTest();

protected:
	virtual void Step() override;

private:
	enum class EPhase : uint8
	{
		Setup,
		VaultCheck,
		MantleCheck,
		ClimbCheck,
		StowInterrupt,
		SlideCheck,
		SlideConfirm,
		MeshDrift,
		Finished
	};

	EPhase Current() const { return static_cast<EPhase>(GetPhase()); }
	void Go(EPhase Next, float Dwell) { Advance(static_cast<int32>(Next), Dwell); }

	/**
	 * Places the operative a stride in front of an obstacle facing it, then
	 * reports what the traversal scan makes of it and starts the traversal.
	 * Teleporting rather than running there keeps each verb an isolated,
	 * repeatable check.
	 */
	bool CheckTraversalAt(const FVector& StandLocation, float FacingYaw,
		EEOTraversalType Expected, const TCHAR* Label);

	/** The checks every verb shares once its dwell has passed. */
	void CheckTraversalEnded(const TCHAR* Verb);
};
