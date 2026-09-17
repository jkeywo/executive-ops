#pragma once

#include "CoreMinimal.h"
#include "EOMissionTypes.generated.h"

/**
 * The framework loop from the milestone document, as a single linear state machine.
 *
 * CITY MAP -> FLY -> APPROACH -> DEPLOY -> GROUND -> OBJECTIVE -> EXTRACT -> FLY AWAY
 *
 * M0 only drives Inactive/InFlight/OnGround. The remaining states exist so that
 * M3, M6 and M7 have somewhere to plug in without reshaping the enum.
 */
UENUM(BlueprintType)
enum class EEOMissionState : uint8
{
	/** No mission selected. Free flight. */
	Inactive		UMETA(DisplayName = "Inactive"),

	/** Mission selected, player flying towards the site. */
	InFlight		UMETA(DisplayName = "In Flight"),

	/** Inside the hover volume, deployment sequence running. (M3) */
	Deploying		UMETA(DisplayName = "Deploying"),

	/** Operative possessed, mission underway. */
	OnGround		UMETA(DisplayName = "On Ground"),

	/** Objective interacted with, extraction now available. (M6) */
	ObjectiveComplete	UMETA(DisplayName = "Objective Complete"),

	/** Extraction called, aircraft arriving. (M7) */
	Extracting		UMETA(DisplayName = "Extracting"),

	/** Player back in the aircraft, mission closed out. */
	Complete		UMETA(DisplayName = "Complete"),

	/** Operative died. */
	Failed			UMETA(DisplayName = "Failed")
};

/** Which pawn the player is currently driving. */
UENUM(BlueprintType)
enum class EEOControlMode : uint8
{
	None		UMETA(DisplayName = "None"),
	Aircraft	UMETA(DisplayName = "Aircraft"),
	Operative	UMETA(DisplayName = "Operative")
};
