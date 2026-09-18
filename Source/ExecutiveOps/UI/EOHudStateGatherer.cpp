#include "UI/EOHudStateGatherer.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"
#include "Core/EOPlayerController.h"
#include "Mission/EOExtractionZone.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOMissionSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

bool UEOHudStateGatherer::Gather(AEOPlayerController* Controller, FEOHUDState& Out)
{
	if (!Controller)
	{
		return false;
	}

	Out.Controller = Controller;
	Out.Pawn = Controller->GetPawn();
	if (!Out.Pawn)
	{
		return false;
	}

	UWorld* World = Controller->GetWorld();
	Out.Mission = World ? World->GetSubsystem<UEOMissionSubsystem>() : nullptr;
	EnsureMissionBinding(Out.Mission);

	RefreshActorCache(World);

	Out.bFlying = Controller->GetControlMode() == EEOControlMode::Aircraft;

	Out.Operative = Cast<AEOOperativeCharacter>(Out.Pawn);
	Out.Aircraft = Cast<AEOAircraftPawn>(Out.Pawn);
	if (!Out.Aircraft)
	{
		// On the ground the craft is still part of the mission, so the asset slot
		// keeps reporting it.
		Out.Aircraft = CachedAircraft.Get();
	}

	Out.Site = Out.Mission ? Out.Mission->GetSelectedSite() : nullptr;
	Out.Extraction = CachedExtraction.Get();

	const FVector ViewerLocation = Out.Pawn->GetActorLocation();
	for (const TWeakObjectPtr<AEOGuardCharacter>& Weak : CachedGuards)
	{
		AEOGuardCharacter* Guard = Weak.Get();
		if (Guard && !Guard->IsDead())
		{
			Out.Guards.Add(Guard);
		}
	}

	Out.Guards.Sort([&ViewerLocation](const AEOGuardCharacter& A, const AEOGuardCharacter& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), ViewerLocation)
			< FVector::DistSquared(B.GetActorLocation(), ViewerLocation);
	});

	return true;
}

void UEOHudStateGatherer::EnsureMissionBinding(UEOMissionSubsystem* Mission)
{
	if (!Mission || BoundMission.Get() == Mission)
	{
		return;
	}

	// OnMissionStateChanged had no subscriber at all before this: the interface
	// polled GetMissionState() every frame and picked up new actors whenever the
	// scan interval next came round, which could be a second after the player
	// completed an objective.
	BoundMission = Mission;
	Mission->OnMissionStateChanged.AddDynamic(this, &UEOHudStateGatherer::HandleMissionStateChanged);
}

void UEOHudStateGatherer::HandleMissionStateChanged(EEOMissionState OldState, EEOMissionState NewState)
{
	// A transition is exactly when the set of things worth drawing changes - an
	// extraction zone becomes relevant, a site stops being. Rescan now rather
	// than waiting out the interval.
	InvalidateActorCache();
}

void UEOHudStateGatherer::RefreshActorCache(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastActorScanTime < ActorScanInterval)
	{
		return;
	}
	LastActorScanTime = Now;

	CachedGuards.Reset();
	for (TActorIterator<AEOGuardCharacter> It(World); It; ++It)
	{
		CachedGuards.Add(*It);
	}

	if (!CachedAircraft.IsValid())
	{
		for (TActorIterator<AEOAircraftPawn> It(World); It; ++It)
		{
			CachedAircraft = *It;
			break;
		}
	}

	if (!CachedExtraction.IsValid())
	{
		for (TActorIterator<AEOExtractionZone> It(World); It; ++It)
		{
			CachedExtraction = *It;
			break;
		}
	}
}
