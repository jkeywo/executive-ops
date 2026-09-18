#include "EOCheatManagerExtension.h"

#include "ExecutiveOpsDev.h"

#include "Core/EOPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/EOMissionSubsystem.h"
#include "UI/EODebugHUD.h"
#include "UI/EONavigationHUD.h"
#include "UI/EOPlayerHUD.h"
#include "Mission/EOMissionSite.h"
#include "Interfaces/EOAircraftControlInterface.h"

void UEOCheatManagerExtension::EOReset()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetPlayerController()))
	{
		PC->EOReset();
	}
}

void UEOCheatManagerExtension::EORestartLevel()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/true)));
	}
}

void UEOCheatManagerExtension::EOPossessAircraft()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetPlayerController()))
	{
		PC->PossessAircraft();
	}
}

void UEOCheatManagerExtension::EOPossessOperative()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetPlayerController()))
	{
		PC->PossessOperative();
	}
}

void UEOCheatManagerExtension::EODeploy()
{
	AEOPlayerController* PC = Cast<AEOPlayerController>(GetPlayerController());
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
		UE_LOG(LogExecutiveOpsDev, Warning, TEXT("EODeploy failed."));
	}
}

void UEOCheatManagerExtension::EOExtract()
{
	if (AEOPlayerController* PC = Cast<AEOPlayerController>(GetPlayerController()))
	{
		if (!PC->RequestExtraction())
		{
			UE_LOG(LogExecutiveOpsDev, Warning, TEXT("EOExtract failed."));
		}
	}
}

void UEOCheatManagerExtension::EOToggleDebugHUD()
{
	if (const APlayerController* PC = GetPlayerController())
	{
		if (AEODebugHUD* HUD = Cast<AEODebugHUD>(PC->GetHUD()))
		{
			HUD->ToggleVisible();
		}
	}
}

void UEOCheatManagerExtension::EOToggleHUD()
{
	if (const APlayerController* PC = GetPlayerController())
	{
		if (AEOPlayerHUD* HUD = Cast<AEOPlayerHUD>(PC->GetHUD()))
		{
			HUD->ToggleFrame();
		}
	}
}

void UEOCheatManagerExtension::EOToggleMap()
{
	if (const APlayerController* PC = GetPlayerController())
	{
		if (AEONavigationHUD* HUD = Cast<AEONavigationHUD>(PC->GetHUD()))
		{
			HUD->ToggleMap();
		}
	}
}

void UEOCheatManagerExtension::EOSelectMission()
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
		UE_LOG(LogExecutiveOpsDev, Warning, TEXT("EOSelectMission: no mission site in this level."));
		return;
	}

	Mission->StartMission();
}

void UEOCheatManagerExtension::EOMissionState()
{
	if (const UWorld* World = GetWorld())
	{
		if (const UEOMissionSubsystem* Mission = World->GetSubsystem<UEOMissionSubsystem>())
		{
			UE_LOG(LogExecutiveOpsDev, Log, TEXT("Mission state: %s"), *Mission->GetMissionStateName());
		}
	}
}
