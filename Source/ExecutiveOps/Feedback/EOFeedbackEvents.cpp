#include "Feedback/EOFeedbackEvents.h"

namespace EOFeedbackEvents
{
	const FName Aircraft_Accelerate(TEXT("Aircraft_Accelerate"));
	const FName Aircraft_BrakeHard(TEXT("Aircraft_BrakeHard"));
	const FName Aircraft_LateralBurst(TEXT("Aircraft_LateralBurst"));
	const FName Aircraft_Collision(TEXT("Aircraft_Collision"));
	const FName Aircraft_Scrape(TEXT("Aircraft_Scrape"));

	const FName Deploy_Ready(TEXT("Deploy_Ready"));
	const FName Deploy_Launch(TEXT("Deploy_Launch"));
	const FName Deploy_Land(TEXT("Deploy_Land"));

	const FName Parkour_Vault(TEXT("Parkour_Vault"));
	const FName Parkour_Mantle(TEXT("Parkour_Mantle"));
	const FName Parkour_Climb(TEXT("Parkour_Climb"));
	const FName Parkour_Contact(TEXT("Parkour_Contact"));
	const FName Parkour_Slide(TEXT("Parkour_Slide"));
	const FName Parkour_SlideEnd(TEXT("Parkour_SlideEnd"));
	const FName Move_LandLight(TEXT("Move_LandLight"));
	const FName Move_LandHard(TEXT("Move_LandHard"));

	const FName Takedown_Commit(TEXT("Takedown_Commit"));
	const FName Takedown_Impact(TEXT("Takedown_Impact"));

	const FName Pistol_Fire(TEXT("Pistol_Fire"));
	const FName Pistol_HitHard(TEXT("Pistol_HitHard"));
	const FName Pistol_HitMetal(TEXT("Pistol_HitMetal"));
	const FName Pistol_HitCharacter(TEXT("Pistol_HitCharacter"));

	const FName Guard_Fire(TEXT("Guard_Fire"));
	const FName Guard_NearMiss(TEXT("Guard_NearMiss"));
	const FName Player_Damaged(TEXT("Player_Damaged"));

	const FName Guard_Suspicious(TEXT("Guard_Suspicious"));
	const FName Guard_DetectConfirmed(TEXT("Guard_DetectConfirmed"));
	const FName Guard_Searching(TEXT("Guard_Searching"));
	const FName Guard_LostContact(TEXT("Guard_LostContact"));
	const FName Guard_Death(TEXT("Guard_Death"));

	const FName Objective_Complete(TEXT("Objective_Complete"));
	const FName Extraction_Call(TEXT("Extraction_Call"));
	const FName Extraction_Board(TEXT("Extraction_Board"));

	const FName UI_ActionAvailable(TEXT("UI_ActionAvailable"));
	const FName UI_ActionUnavailable(TEXT("UI_ActionUnavailable"));
}
