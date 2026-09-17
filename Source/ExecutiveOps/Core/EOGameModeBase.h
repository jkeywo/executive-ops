#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EOGameModeBase.generated.h"

/**
 * Wires the project's controller, state and HUD together. Deliberately holds no rules:
 * mission logic lives in UEOMissionSubsystem so it survives level transitions.
 */
UCLASS()
class EXECUTIVEOPS_API AEOGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEOGameModeBase();
};
