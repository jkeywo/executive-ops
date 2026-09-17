#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EOSelfTest.generated.h"

class AEOPlayerController;

/**
 * Automated check of M0's "done when": the project boots, the player possesses the
 * aircraft, and the aircraft -> operative -> aircraft transitions drive the mission
 * state machine correctly.
 *
 * Runs on a short timer after BeginPlay when the game is launched with -EOSelfTest,
 * because the transitions need a live world and a possessed pawn. Add -EOSelfTestExit
 * to quit with an exit code reflecting the result, for CI.
 */
UCLASS()
class EXECUTIVEOPS_API UEOSelfTest : public UObject
{
	GENERATED_BODY()

public:
	/** Runs every check, logs each one, returns true only if all passed. */
	static bool Run(AEOPlayerController* Controller);

	/** True if -EOSelfTest was passed on the command line. */
	static bool IsRequested();

	/** True if -EOSelfTestExit was passed on the command line. */
	static bool ShouldExitAfterRun();
};
