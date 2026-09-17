#include "Core/EOGameModeBase.h"

#include "Character/EOOperativeCharacter.h"
#include "Core/EOGameStateBase.h"
#include "Core/EOPlayerController.h"
#include "UI/EOPlayerHUD.h"

AEOGameModeBase::AEOGameModeBase()
{
	PlayerControllerClass = AEOPlayerController::StaticClass();
	GameStateClass = AEOGameStateBase::StaticClass();
	// Draws the player frame; it still inherits the waypoint, the tactical map and
	// the debug readout from the HUDs it is built on.
	HUDClass = AEOPlayerHUD::StaticClass();

	// The controller possesses the real pawn on BeginPlay; this is only the boot pawn.
	DefaultPawnClass = AEOOperativeCharacter::StaticClass();
}
