#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * The events the game fires, in one place.
 *
 * Gameplay code names an event; the preset set decides what the event does. That
 * indirection is the whole point of the feedback system: retuning a verb means
 * editing data, and a missing preset means silence rather than a crash.
 *
 * These are native gameplay tags rather than FNames, so the hierarchy is real:
 * Feedback.Pistol.Fire and Feedback.Pistol.HitMetal are siblings a preset can
 * one day match as a family, and the editor autocompletes them. Declaring them
 * natively means they register at module load with no ini list to maintain.
 *
 * FNativeGameplayTag converts implicitly to FGameplayTag, so call sites read the
 * same as they always did.
 *
 * These match the names in GDD/executive-ops-m8-polish-work-brief.md §4.
 */
namespace EOFeedbackEvents
{
	// Flight
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aircraft_Accelerate);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aircraft_BrakeHard);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aircraft_LateralBurst);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aircraft_Collision);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aircraft_Scrape);

	// Deployment
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Deploy_Ready);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Deploy_Launch);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Deploy_Land);

	// Ground movement
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Vault);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Mantle);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Climb);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Contact);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_Slide);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parkour_SlideEnd);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move_LandLight);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move_LandHard);

	// Takedown
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Takedown_Commit);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Takedown_Impact);

	// Pistol
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pistol_Fire);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pistol_HitHard);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pistol_HitMetal);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pistol_HitCharacter);

	// Incoming fire
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_Fire);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_NearMiss);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Damaged);

	// Detection ladder
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_Suspicious);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_DetectConfirmed);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_Searching);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_LostContact);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Guard_Death);

	// Mission
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Objective_Complete);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Extraction_Call);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Extraction_Board);

	// UI
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_ActionAvailable);
	EXECUTIVEOPS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_ActionUnavailable);
}
