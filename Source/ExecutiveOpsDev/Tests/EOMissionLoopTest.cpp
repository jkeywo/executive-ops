#include "Tests/EOMissionLoopTest.h"

#include "ExecutiveOpsDev.h"

#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Core/EOPlayerController.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EODeployableInterface.h"
#include "Interfaces/EOExtractionInterface.h"
#include "Interfaces/EOMissionTypes.h"
#include "Mission/EOExtractionZone.h"
#include "Mission/EOMissionSubsystem.h"
#include "Mission/EOObjectiveTerminal.h"

#include "Engine/World.h"

AEOMissionLoopTest::AEOMissionLoopTest()
{
	// Two runs, each waiting up to thirty seconds for a real flight in.
	TimeLimit = 100.f;
}

bool AEOMissionLoopTest::EnterGroundMission()
{
	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	if (!Mission)
	{
		return false;
	}

	// Stands in for the flight and the drop, which the flight sequence covers.
	Mission->ResetMission();
	return Mission->StartMission()
		&& Mission->BeginDeployment()
		&& Mission->CompleteDeployment();
}

void AEOMissionLoopTest::ResetEncounter()
{
	if (AEOGuardCharacter* Guard = GetGuard())
	{
		Guard->ResetGuard();
	}
	if (AEOOperativeCharacter* Op = GetOperative())
	{
		if (UEOHealthComponent* Health = Op->GetHealth())
		{
			Health->Revive();
		}
	}
}

void AEOMissionLoopTest::Step()
{
	AEOPlayerController* Controller = GetController();
	if (!Controller)
	{
		Check(false, TEXT("no player controller"));
		Done();
		return;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();

	switch (Current())
	{
	case EPhase::Setup:
	{
		// The sequence drives the mission directly, so make sure the player is
		// actually holding the operative before each run.
		if (!GetOperative())
		{
			Controller->PossessOperative();
		}
		if (AEOOperativeCharacter* Ready = GetOperative())
		{
			IEODeployableInterface::Execute_SetStowed(Ready, false);
		}

		AEOOperativeCharacter* Op = GetOperative();
		AEOObjectiveTerminal* Objective = GetObjective();
		AEOExtractionZone* Zone = GetExtractionZone();

		Check(Objective != nullptr, FString::Printf(TEXT("%s the arena has an objective"), *Pass()));
		Check(Zone != nullptr, FString::Printf(TEXT("%s the arena has an extraction zone"), *Pass()));

		if (!Op || !Objective || !Zone)
		{
			Done();
			return;
		}

		ResetEncounter();
		Check(EnterGroundMission(), FString::Printf(TEXT("%s insertion reaches OnGround"), *Pass()));

		// The objective must be fresh, or the second run has nothing to do.
		Check(!Objective->IsComplete(), FString::Printf(TEXT("%s objective starts incomplete"), *Pass()));

		// Extraction must be dark until the objective is done, or the player is
		// being told to leave before they have done anything.
		Check(!IEOExtractionInterface::Execute_IsExtractionAvailable(Zone),
			FString::Printf(TEXT("%s extraction is unavailable before the objective"), *Pass()));

		// Standing at the extraction point early does nothing.
		Op->SetActorLocation(Zone->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		Check(Op->FindInteractable() == nullptr,
			FString::Printf(TEXT("%s nothing to interact with at a dormant extraction"), *Pass()));

		Go(EPhase::Objective, 0.2f);
		return;
	}

	case EPhase::Objective:
	{
		AEOOperativeCharacter* Op = GetOperative();
		AEOObjectiveTerminal* Objective = GetObjective();
		AEOExtractionZone* Zone = GetExtractionZone();
		if (!Op || !Objective || !Zone)
		{
			Done();
			return;
		}

		// Cross the arena to the terminal.
		Op->SetActorLocation(Objective->GetActorLocation() + FVector(200.f, 0.f, 0.f),
			false, nullptr, ETeleportType::TeleportPhysics);

		Check(Op->FindInteractable() == Objective,
			FString::Printf(TEXT("%s the terminal offers itself when reached"), *Pass()));
		Check(Op->TryInteract(), FString::Printf(TEXT("%s interacting completes the objective"), *Pass()));
		Check(Objective->IsComplete(), FString::Printf(TEXT("%s objective reports complete"), *Pass()));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::ObjectiveComplete,
			FString::Printf(TEXT("%s mission reaches ObjectiveComplete"), *Pass()));

		// Doing it twice must not double-fire the transition.
		Check(!Op->TryInteract(), FString::Printf(TEXT("%s the objective cannot be done twice"), *Pass()));

		Check(IEOExtractionInterface::Execute_IsExtractionAvailable(Zone),
			FString::Printf(TEXT("%s extraction arms once the objective is done"), *Pass()));

		Go(EPhase::Extract, 0.2f);
		return;
	}

	case EPhase::Extract:
	{
		AEOOperativeCharacter* Op = GetOperative();
		AEOExtractionZone* Zone = GetExtractionZone();
		if (!Op || !Zone)
		{
			Done();
			return;
		}

		// Run back across the arena to the pad.
		Op->SetActorLocation(Zone->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		Check(Zone->IsWithinZone(Op), FString::Printf(TEXT("%s standing in the extraction zone"), *Pass()));
		Check(Op->FindInteractable() == Zone,
			FString::Printf(TEXT("%s the pad offers extraction"), *Pass()));

		// Park the aircraft a long way off, so the arrival is a real flight in
		// rather than a craft that happens to already be overhead.
		if (APawn* Inbound = FindAircraftInLevel())
		{
			Inbound->SetActorLocation(Op->GetActorLocation() + FVector(-9000.f, -6000.f, 5000.f),
				false, nullptr, ETeleportType::TeleportPhysics);
			IEOAircraftControlInterface::Execute_ResetFlightState(Inbound);
		}

		Check(Op->TryInteract(), FString::Printf(TEXT("%s calling extraction works"), *Pass()));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Extracting,
			FString::Printf(TEXT("%s mission enters Extracting"), *Pass()));
		Check(Controller->IsExtractionInbound(),
			FString::Printf(TEXT("%s the aircraft is inbound"), *Pass()));
		Check(Controller->GetControlMode() == EEOControlMode::Operative,
			FString::Printf(TEXT("%s the operative stays playable while it comes"), *Pass()));

		DistanceSample = Controller->GetExtractionDistance();
		Check(DistanceSample > 50.f,
			FString::Printf(TEXT("%s the aircraft starts a long way out (%.0fm)"), *Pass(), DistanceSample));

		// Polled, not timed: the mission re-arms itself a few seconds after it
		// completes, so a fixed dwell would sample the re-armed state instead.
		Go(EPhase::Pickup, 0.f);
		return;
	}

	case EPhase::Pickup:
	{
		// Wait for the arrival, up to a generous bound.
		if (Controller->IsExtractionInbound() && GetPhaseElapsed() < 30.f)
		{
			return;
		}

		Check(!Controller->IsExtractionInbound(),
			FString::Printf(TEXT("%s the extraction finishes"), *Pass()));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			FString::Printf(TEXT("%s the pickup returns control to the aircraft"), *Pass()));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Complete,
			FString::Printf(TEXT("%s mission reaches Complete"), *Pass()));

		// And the loop closes: the craft the player is handed back must fly.
		if (APawn* Departing = GetAircraftPawn())
		{
			IEOAircraftControlInterface::Execute_SetFlightInput(Departing, FVector(1.f, 0.f, 0.f));
		}

		if (Attempt >= 2)
		{
			Go(EPhase::Finished, 1.5f);
			return;
		}

		// The whole point of M6: it has to be playable again without debug help.
		UE_LOG(LogExecutiveOpsDev, Display,
			TEXT("[SelfTest] -- resetting and running the mission again --"));
		Controller->EOReset();
		Attempt = 2;
		Go(EPhase::SecondRun, 0.5f);
		return;
	}

	case EPhase::SecondRun:
	{
		// EOReset put the player back in the aircraft; the sequence needs the
		// operative again before the second run can start.
		Check(Controller->PossessOperative(), TEXT("run 2: can possess the operative again"));
		if (AEOOperativeCharacter* Op = GetOperative())
		{
			IEODeployableInterface::Execute_SetStowed(Op, false);
		}
		Go(EPhase::Setup, 0.2f);
		return;
	}

	case EPhase::Finished:
	default:
		Done();
		return;
	}
}
