#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

EXECUTIVEOPSDEV_API DECLARE_LOG_CATEGORY_EXTERN(LogExecutiveOpsDev, Log, All);

/**
 * Attaches the development tools to the running game without the game knowing.
 *
 * Both hooks are the engine's own: cheats arrive through
 * UCheatManager::RegisterForOnCheatManagerCreated, and the self-test starts off
 * FGameModeEvents::OnGameModePostLoginEvent. Neither needs a line of code in the
 * game module, which is what lets this module be dropped from a shipping build
 * without anything missing it.
 */
class FExecutiveOpsDevModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FDelegateHandle CheatManagerHandle;
	FDelegateHandle PostLoginHandle;
};
