#pragma once

#include "CoreMinimal.h"

/**
 * The event names the game fires, in one place.
 *
 * Gameplay code names an event; the preset set decides what the event does. That
 * indirection is the whole point of the feedback system: retuning a verb means
 * editing data, and a missing preset means silence rather than a crash.
 *
 * These match the names in GDD/executive-ops-m8-polish-work-brief.md §4.
 */
namespace EOFeedbackEvents
{
	// Flight
	EXECUTIVEOPS_API extern const FName Aircraft_Accelerate;
	EXECUTIVEOPS_API extern const FName Aircraft_BrakeHard;
	EXECUTIVEOPS_API extern const FName Aircraft_LateralBurst;
	EXECUTIVEOPS_API extern const FName Aircraft_Collision;
	EXECUTIVEOPS_API extern const FName Aircraft_Scrape;

	// Deployment
	EXECUTIVEOPS_API extern const FName Deploy_Ready;
	EXECUTIVEOPS_API extern const FName Deploy_Launch;
	EXECUTIVEOPS_API extern const FName Deploy_Land;

	// Ground movement
	EXECUTIVEOPS_API extern const FName Parkour_Vault;
	EXECUTIVEOPS_API extern const FName Parkour_Mantle;
	EXECUTIVEOPS_API extern const FName Parkour_Climb;
	EXECUTIVEOPS_API extern const FName Parkour_Contact;
	EXECUTIVEOPS_API extern const FName Parkour_Slide;
	EXECUTIVEOPS_API extern const FName Parkour_SlideEnd;
	EXECUTIVEOPS_API extern const FName Move_LandLight;
	EXECUTIVEOPS_API extern const FName Move_LandHard;

	// Takedown
	EXECUTIVEOPS_API extern const FName Takedown_Commit;
	EXECUTIVEOPS_API extern const FName Takedown_Impact;

	// Pistol
	EXECUTIVEOPS_API extern const FName Pistol_Fire;
	EXECUTIVEOPS_API extern const FName Pistol_HitHard;
	EXECUTIVEOPS_API extern const FName Pistol_HitMetal;
	EXECUTIVEOPS_API extern const FName Pistol_HitCharacter;

	// Incoming fire
	EXECUTIVEOPS_API extern const FName Guard_Fire;
	EXECUTIVEOPS_API extern const FName Guard_NearMiss;
	EXECUTIVEOPS_API extern const FName Player_Damaged;

	// Detection ladder
	EXECUTIVEOPS_API extern const FName Guard_Suspicious;
	EXECUTIVEOPS_API extern const FName Guard_DetectConfirmed;
	EXECUTIVEOPS_API extern const FName Guard_Searching;
	EXECUTIVEOPS_API extern const FName Guard_LostContact;
	EXECUTIVEOPS_API extern const FName Guard_Death;

	// Mission
	EXECUTIVEOPS_API extern const FName Objective_Complete;
	EXECUTIVEOPS_API extern const FName Extraction_Call;
	EXECUTIVEOPS_API extern const FName Extraction_Board;

	// UI
	EXECUTIVEOPS_API extern const FName UI_ActionAvailable;
	EXECUTIVEOPS_API extern const FName UI_ActionUnavailable;
}
