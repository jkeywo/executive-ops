#pragma once

#include "CoreMinimal.h"

class AEOPlayerController;
class APawn;
class AEOAircraftPawn;
class AEOOperativeCharacter;
class UEOMissionSubsystem;
class AEOMissionSite;
class AEOExtractionZone;
class AEOGuardCharacter;

/**
 * Everything the interface needs, gathered once per frame.
 *
 * The single seam between the game and its presentation. Three HUD classes used
 * to read the world independently, each re-casting the controller, re-fetching
 * the mission subsystem and re-deriving the same nine readouts; one of them
 * answered "is the player flying" two different ways three lines apart.
 *
 * Nothing that consumes this is allowed to reach past it and query the world
 * itself. That is what keeps the per-frame cost paid once, and what will let the
 * same state drive both the viewport and the in-world panel.
 */
struct FEOHUDState
{
	AEOPlayerController* Controller = nullptr;
	APawn* Pawn = nullptr;

	/** The craft, whether the player is in it or it is waiting overhead. */
	AEOAircraftPawn* Aircraft = nullptr;
	AEOOperativeCharacter* Operative = nullptr;

	UEOMissionSubsystem* Mission = nullptr;
	AEOMissionSite* Site = nullptr;
	AEOExtractionZone* Extraction = nullptr;

	/** Guards that are not dead, nearest first. */
	TArray<AEOGuardCharacter*> Guards;

	/** True while the player is flying rather than on foot. */
	bool bFlying = false;
};
