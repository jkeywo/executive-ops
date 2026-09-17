#include "Feedback/EOFeedbackEvents.h"

/**
 * Tag strings are the authority the preset asset keys off, so they are not free
 * to rename: Scripts/m8_build_feedback_presets.py writes the same strings, and a
 * mismatch shows up as silence rather than as an error.
 */
namespace EOFeedbackEvents
{
	UE_DEFINE_GAMEPLAY_TAG(Aircraft_Accelerate, "Feedback.Aircraft.Accelerate");
	UE_DEFINE_GAMEPLAY_TAG(Aircraft_BrakeHard, "Feedback.Aircraft.BrakeHard");
	UE_DEFINE_GAMEPLAY_TAG(Aircraft_LateralBurst, "Feedback.Aircraft.LateralBurst");
	UE_DEFINE_GAMEPLAY_TAG(Aircraft_Collision, "Feedback.Aircraft.Collision");
	UE_DEFINE_GAMEPLAY_TAG(Aircraft_Scrape, "Feedback.Aircraft.Scrape");

	UE_DEFINE_GAMEPLAY_TAG(Deploy_Ready, "Feedback.Deploy.Ready");
	UE_DEFINE_GAMEPLAY_TAG(Deploy_Launch, "Feedback.Deploy.Launch");
	UE_DEFINE_GAMEPLAY_TAG(Deploy_Land, "Feedback.Deploy.Land");

	UE_DEFINE_GAMEPLAY_TAG(Parkour_Vault, "Feedback.Parkour.Vault");
	UE_DEFINE_GAMEPLAY_TAG(Parkour_Mantle, "Feedback.Parkour.Mantle");
	UE_DEFINE_GAMEPLAY_TAG(Parkour_Climb, "Feedback.Parkour.Climb");
	UE_DEFINE_GAMEPLAY_TAG(Parkour_Contact, "Feedback.Parkour.Contact");
	UE_DEFINE_GAMEPLAY_TAG(Parkour_Slide, "Feedback.Parkour.Slide");
	UE_DEFINE_GAMEPLAY_TAG(Parkour_SlideEnd, "Feedback.Parkour.SlideEnd");
	UE_DEFINE_GAMEPLAY_TAG(Move_LandLight, "Feedback.Move.LandLight");
	UE_DEFINE_GAMEPLAY_TAG(Move_LandHard, "Feedback.Move.LandHard");

	UE_DEFINE_GAMEPLAY_TAG(Takedown_Commit, "Feedback.Takedown.Commit");
	UE_DEFINE_GAMEPLAY_TAG(Takedown_Impact, "Feedback.Takedown.Impact");

	UE_DEFINE_GAMEPLAY_TAG(Pistol_Fire, "Feedback.Pistol.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Pistol_HitHard, "Feedback.Pistol.HitHard");
	UE_DEFINE_GAMEPLAY_TAG(Pistol_HitMetal, "Feedback.Pistol.HitMetal");
	UE_DEFINE_GAMEPLAY_TAG(Pistol_HitCharacter, "Feedback.Pistol.HitCharacter");

	UE_DEFINE_GAMEPLAY_TAG(Guard_Fire, "Feedback.Guard.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Guard_NearMiss, "Feedback.Guard.NearMiss");
	UE_DEFINE_GAMEPLAY_TAG(Player_Damaged, "Feedback.Player.Damaged");

	UE_DEFINE_GAMEPLAY_TAG(Guard_Suspicious, "Feedback.Guard.Suspicious");
	UE_DEFINE_GAMEPLAY_TAG(Guard_DetectConfirmed, "Feedback.Guard.DetectConfirmed");
	UE_DEFINE_GAMEPLAY_TAG(Guard_Searching, "Feedback.Guard.Searching");
	UE_DEFINE_GAMEPLAY_TAG(Guard_LostContact, "Feedback.Guard.LostContact");
	UE_DEFINE_GAMEPLAY_TAG(Guard_Death, "Feedback.Guard.Death");

	UE_DEFINE_GAMEPLAY_TAG(Objective_Complete, "Feedback.Objective.Complete");
	UE_DEFINE_GAMEPLAY_TAG(Extraction_Call, "Feedback.Extraction.Call");
	UE_DEFINE_GAMEPLAY_TAG(Extraction_Board, "Feedback.Extraction.Board");

	UE_DEFINE_GAMEPLAY_TAG(UI_ActionAvailable, "Feedback.UI.ActionAvailable");
	UE_DEFINE_GAMEPLAY_TAG(UI_ActionUnavailable, "Feedback.UI.ActionUnavailable");
}
