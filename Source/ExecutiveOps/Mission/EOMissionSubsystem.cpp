#include "Mission/EOMissionSubsystem.h"

#include "ExecutiveOps.h"
#include "EngineUtils.h"
#include "Interfaces/EOMissionParticipantInterface.h"

bool UEOMissionSubsystem::StartMission()
{
	return TryTransition(EEOMissionState::Inactive, EEOMissionState::InFlight);
}

bool UEOMissionSubsystem::BeginDeployment()
{
	return TryTransition(EEOMissionState::InFlight, EEOMissionState::Deploying);
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
