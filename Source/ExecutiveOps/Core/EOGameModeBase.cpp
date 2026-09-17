#include "Core/EOGameModeBase.h"

#include "Character/EOOperativeCharacter.h"
#include "Core/EOGameStateBase.h"
#include "Core/EOPlayerController.h"
#include "UI/EODebugHUD.h"

AEOGameModeBase::AEOGameModeBase()
{
	PlayerControllerClass = AEOPlayerController::StaticClass();
	GameStateClass = AEOGameStateBase::StaticClass();
	HUDClass = AEODebugHUD::StaticClass();

	// The controller possesses the real pawn on BeginPlay; this is only the boot pawn.
	DefaultPawnClass = AEOOperativeCharacter::StaticClass();
}
