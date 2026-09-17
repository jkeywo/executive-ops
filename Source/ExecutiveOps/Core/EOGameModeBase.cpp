#include "Core/EOGameModeBase.h"

#include "Character/EOOperativeCharacter.h"
#include "Core/EOGameStateBase.h"
#include "Core/EOPlayerController.h"
#include "UI/EONavigationHUD.h"

AEOGameModeBase::AEOGameModeBase()
{
	PlayerControllerClass = AEOPlayerController::StaticClass();
	GameStateClass = AEOGameStateBase::StaticClass();
	HUDClass = AEONavigationHUD::StaticClass();

	// The controller possesses the real pawn on BeginPlay; this is only the boot pawn.
	DefaultPawnClass = AEOOperativeCharacter::StaticClass();
}
