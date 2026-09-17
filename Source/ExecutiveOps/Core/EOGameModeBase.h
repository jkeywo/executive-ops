#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EOGameModeBase.generated.h"

/**
 * Wires the project's controller, state and HUD together. Deliberately holds no rules:
 * mission logic lives in UEOMissionSubsystem, which owns it for the lifetime of the
 * world. A mission is per-world by definition - see docs/adr/0002.
 */
UCLASS()
class EXECUTIVEOPS_API AEOGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEOGameModeBase();
};
