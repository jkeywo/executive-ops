#include "Mission/EOMissionSubsystem.h"

#include "ExecutiveOps.h"
#include "EngineUtils.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/EOMissionParticipantInterface.h"
#include "Mission/EOMissionSite.h"

bool UEOMissionSubsystem::SelectSite(AEOMissionSite* Site)
{
	if (!Site)
	{
		return false;
	}

	// Changing target mid-mission would leave the ground phase pointing at the
	// wrong place, so selection is only allowed before committing.
	if (MissionState != EEOMissionState::Inactive && MissionState != EEOMissionState::InFlight)
	{
		UE_LOG(LogExecutiveOps, Warning,
			TEXT("Site selection rejected: mission already underway (%s)."), *GetMissionStateName());
		return false;
	}

	SelectedSite = Site;
	UE_LOG(LogExecutiveOps, Log, TEXT("Mission site selected: %s"), *Site->GetDisplayName().ToString());
	return true;
}

AEOMissionSite* UEOMissionSubsystem::SelectDefaultSite()
{
	if (SelectedSite)
	{
		return SelectedSite;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AEOMissionSite> It(World); It; ++It)
	{
		// Stop at the first site that is actually accepted. Breaking regardless
		// would report "no site in this level" for what is really a refused
		// selection, which is a misleading diagnosis.
		if (SelectSite(*It))
		{
			break;
		}
	}

	return SelectedSite;
}

bool UEOMissionSubsystem::StartMission()
{
	return TryTransition(EEOMissionState::Inactive, EEOMissionState::InFlight);
}

bool UEOMissionSubsystem::BeginDeployment()
{
	return TryTransition(EEOMissionState::InFlight, EEOMissionState::Deploying);
}

bool UEOMissionSubsystem::AbortDeployment()
{
	return TryTransition(EEOMissionState::Deploying, EEOMissionState::InFlight);
}

bool UEOMissionSubsystem::CompleteDeployment()
{
	return TryTransition(EEOMissionState::Deploying, EEOMissionState::OnGround);
}

bool UEOMissionSubsystem::CompleteObjective()
{
	return TryTransition(EEOMissionState::OnGround, EEOMissionState::ObjectiveComplete);
}

bool UEOMissionSubsystem::BeginExtraction()
{
	return TryTransition(EEOMissionState::ObjectiveComplete, EEOMissionState::Extracting);
}

bool UEOMissionSubsystem::CompleteMission()
{
	return TryTransition(EEOMissionState::Extracting, EEOMissionState::Complete);
}

bool UEOMissionSubsystem::FailMission()
{
	if (MissionState == EEOMissionState::Inactive
		|| MissionState == EEOMissionState::Complete
		|| MissionState == EEOMissionState::Failed)
	{
		return false;
	}

	SetMissionState(EEOMissionState::Failed);
	return true;
}

void UEOMissionSubsystem::ResetMission()
{
	// The selected site survives a reset: the player is being put back at the
	// start of the same mission, not sent back to an empty city.
	SetMissionState(EEOMissionState::Inactive);
}

FString UEOMissionSubsystem::GetMissionStateName() const
{
	return StaticEnum<EEOMissionState>()->GetDisplayNameTextByValue(static_cast<int64>(MissionState)).ToString();
}

bool UEOMissionSubsystem::TryTransition(EEOMissionState From, EEOMissionState To)
{
	if (MissionState != From)
	{
		UE_LOG(LogExecutiveOps, Warning,
			TEXT("Mission transition to %s rejected: expected state %s, actual %s."),
			*StaticEnum<EEOMissionState>()->GetNameStringByValue(static_cast<int64>(To)),
			*StaticEnum<EEOMissionState>()->GetNameStringByValue(static_cast<int64>(From)),
			*StaticEnum<EEOMissionState>()->GetNameStringByValue(static_cast<int64>(MissionState)));
		return false;
	}

	SetMissionState(To);
	return true;
}

void UEOMissionSubsystem::SetMissionState(EEOMissionState NewState)
{
	if (MissionState == NewState)
	{
		return;
	}

	const EEOMissionState OldState = MissionState;
	MissionState = NewState;

	UE_LOG(LogExecutiveOps, Log, TEXT("Mission state: %s -> %s"),
		*StaticEnum<EEOMissionState>()->GetNameStringByValue(static_cast<int64>(OldState)),
		*StaticEnum<EEOMissionState>()->GetNameStringByValue(static_cast<int64>(NewState)));

	OnMissionStateChanged.Broadcast(OldState, NewState);

	// Mission beats are non-diegetic, so they play on the player rather than at a
	// world position - there is nowhere in the level they sensibly come from.
	if (UEOFeedbackSubsystem* Feedback = GetWorld() ? GetWorld()->GetSubsystem<UEOFeedbackSubsystem>() : nullptr)
	{
		FEOFeedbackContext Context;
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				Context.Location = Pawn->GetActorLocation();
				Context.AttachTo = Pawn->GetRootComponent();
			}
		}

		switch (NewState)
		{
		case EEOMissionState::ObjectiveComplete:
			Feedback->Play(EOFeedbackEvents::Objective_Complete, Context);
			break;

		case EEOMissionState::Extracting:
			Feedback->Play(EOFeedbackEvents::Extraction_Call, Context);
			break;

		case EEOMissionState::Complete:
			Feedback->Play(EOFeedbackEvents::Extraction_Board, Context);
			break;

		default:
			break;
		}
	}

	// Push to any actor that cares, so participants do not each need to bind.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->Implements<UEOMissionParticipantInterface>())
			{
				IEOMissionParticipantInterface::Execute_OnMissionStateChanged(*It, OldState, NewState);
			}
		}
	}
}
