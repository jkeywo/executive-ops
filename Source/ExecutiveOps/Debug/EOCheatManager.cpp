#include "Debug/EOCheatManager.h"

#include "Core/EOPlayerController.h"
#include "ExecutiveOps.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/EOMissionSubsystem.h"
#include "UI/EODebugHUD.h"
#include "UI/EONavigationHUD.h"
#include "Mission/EOMissionSite.h"
#include "Interfaces/EOAircraftControlInterface.h"

void UEOCheatManager::EOReset()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetOuterAPlayerController()))
	{
		PC->EOReset();
	}
}

void UEOCheatManager::EORestartLevel()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/true)));
	}
}

void UEOCheatManager::EOPossessAircraft()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetOuterAPlayerController()))
	{
		PC->PossessAircraft();
	}
}

void UEOCheatManager::EOPossessOperative()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetOuterAPlayerController()))
	{
		PC->PossessOperative();
	}
}

void UEOCheatManager::EODeploy()
{
	AEOPlayerController* PC = Cast<AEOPlayerController>(GetOuterAPlayerController());
	if (!PC)
	{
		return;
	}

	// Force the readiness precondition so the transition can be tested from anywhere.
	if (APawn* Pawn = PC->GetPawn())
	{
		if (Pawn->Implements<UEOAircraftControlInterface>())
		{
			IEOAircraftControlInterface::Execute_SetHoverEnabled(Pawn, true);
		}
	}

	if (!PC->RequestDeployment())
	{
		UE_LOG(LogExecutiveOps, Warning, TEXT("EODeploy failed."));
	}
}

void UEOCheatManager::EOExtract()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetOuterAPlayerController()))
	{
		if (!PC->RequestExtraction())
		{
			UE_LOG(LogExecutiveOps, Warning, TEXT("EOExtract failed."));
		}
	}
}

void UEOCheatManager::EOToggleDebugHUD()
{
	if (const APlayerController* PC = GetOuterAPlayerController())
	{
		if (AEODebugHUD* HUD = Cast<AEODebugHUD>(PC->GetHUD()))
		{
			HUD->ToggleVisible();
		}
	}
}

void UEOCheatManager::EOToggleMap()
{
	if (const APlayerController* PC = GetOuterAPlayerController())
	{
		if (AEONavigationHUD* HUD = Cast<AEONavigationHUD>(PC->GetHUD()))
		{
			HUD->ToggleMap();
		}
	}
}

void UEOCheatManager::EOSelectMission()
{
	UWorld* World = GetWorld();
	UEOMissionSubsystem* Mission = World ? World->GetSubsystem<UEOMissionSubsystem>() : nullptr;
	if (!Mission)
	{
		return;
	}

	const AEOMissionSite* Site = Mission->SelectDefaultSite();
	if (!Site)
	{
		UE_LOG(LogExecutiveOps, Warning, TEXT("EOSelectMission: no mission site in this level."));
		return;
	}

	Mission->StartMission();
}

void UEOCheatManager::EOMissionState()
{
	if (const UWorld* World = GetWorld())
	{
		if (const UEOMissionSubsystem* Mission = World->GetSubsystem<UEOMissionSubsystem>())
		{
			UE_LOG(LogExecutiveOps, Log, TEXT("Mission state: %s"), *Mission->GetMissionStateName());
		}
	}
}
