#include "ExecutiveOpsDev.h"

#include "EOCheatManagerExtension.h"
#include "EOSelfTest.h"

#include "Core/EOPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogExecutiveOpsDev);

IMPLEMENT_MODULE(FExecutiveOpsDevModule, ExecutiveOpsDev);

namespace
{
	/**
	 * Starts the self-test once a player controller exists and has a pawn.
	 *
	 * Deferred a beat for the same reason the controller used to defer it: most of
	 * what the suite proves is tick-driven, and the first transitions need a fully
	 * possessed pawn rather than one the game mode is still handing over.
	 */
	void HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
	{
		AEOPlayerController* Controller = Cast<AEOPlayerController>(NewPlayer);
		if (!Controller)
		{
			return;
		}

		FTimerHandle Handle;
		Controller->GetWorldTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateLambda([Controller]()
			{
				if (!IsValid(Controller))
				{
					return;
				}

				UEOSelfTest* SelfTest = NewObject<UEOSelfTest>(Controller, TEXT("EOSelfTest"));

				// Nothing else holds a reference to it. The controller used to, via
				// a UPROPERTY; now that the harness lives outside the game module
				// there is nowhere in the game to put one, so it keeps itself alive
				// and releases on Finish.
				SelfTest->AddToRoot();
				SelfTest->Start(Controller);
			}), 0.5f, false);
	}
}

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

	if (UEOSelfTest::IsRequested())
	{
		PostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddStatic(&HandlePostLogin);

		UE_LOG(LogExecutiveOpsDev, Display,
			TEXT("[SelfTest] requested; waiting for a player controller."));
	}
}

void FExecutiveOpsDevModule::ShutdownModule()
{
	if (PostLoginHandle.IsValid())
	{
		FGameModeEvents::OnGameModePostLoginEvent().Remove(PostLoginHandle);
		PostLoginHandle.Reset();
	}

	if (CheatManagerHandle.IsValid())
	{
		UCheatManager::UnregisterFromOnCheatManagerCreated(CheatManagerHandle);
		CheatManagerHandle.Reset();
	}
}
