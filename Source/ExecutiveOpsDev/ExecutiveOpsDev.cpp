#include "ExecutiveOpsDev.h"

#include "EOCheatManagerExtension.h"

#include "GameFramework/CheatManager.h"

DEFINE_LOG_CATEGORY(LogExecutiveOpsDev);

IMPLEMENT_MODULE(FExecutiveOpsDevModule, ExecutiveOpsDev);

void FExecutiveOpsDevModule::StartupModule()
{
	// Every cheat manager the engine creates gets the extension, including ones
	// created after a level change. The player controller no longer names a
	// CheatClass, so nothing in the game module refers to the cheats at all.
	CheatManagerHandle = UCheatManager::RegisterForOnCheatManagerCreated(
		FOnCheatManagerCreated::FDelegate::CreateLambda([](UCheatManager* CheatManager)
		{
			CheatManager->AddCheatManagerExtension(
				NewObject<UEOCheatManagerExtension>(CheatManager));
		}));
}

void FExecutiveOpsDevModule::ShutdownModule()
{

	if (CheatManagerHandle.IsValid())
	{
		UCheatManager::UnregisterFromOnCheatManagerCreated(CheatManagerHandle);
		CheatManagerHandle.Reset();
	}
}
