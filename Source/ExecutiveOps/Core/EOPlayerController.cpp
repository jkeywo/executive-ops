#include "Core/EOPlayerController.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Character/EOTraversalComponent.h"
#include "Combat/EOGuardCharacter.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "ExecutiveOps.h"
#include "Debug/EOCheatManager.h"
#include "Debug/EOSelfTest.h"
#include "Input/EOInputConfig.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EODeployableInterface.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOObjectiveTerminal.h"
#include "TimerManager.h"
#include "Mission/EOMissionSubsystem.h"

AEOPlayerController::AEOPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	CheatClass = UEOCheatManager::StaticClass();
	AircraftClass = AEOAircraftPawn::StaticClass();
	OperativeClass = AEOOperativeCharacter::StaticClass();
}

void AEOPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InputConfig = NewObject<UEOInputConfig>(this, TEXT("EOInputConfig"));
	InputConfig->BuildRuntimeInput();
}

void AEOPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Boot into whichever pawn the map actually provides, preferring the aircraft.
	if (ResolveAircraft())
	{
		// Resolve the operative up front so it can be stowed out of the way.
		ResolveOperative();
		PossessAircraft();
	}
	else if (ResolveOperative())
	{
		PossessOperative();
	}
	else
	{
		UE_LOG(LogExecutiveOps, Error, TEXT("No aircraft or operative available; player has no pawn."));
	}

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->OnMissionStateChanged.AddDynamic(
			this, &AEOPlayerController::HandleMissionStateChanged);
	}

	if (UEOSelfTest::IsRequested())
	{
		// Deferred a tick: the transitions need a fully possessed pawn.
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AEOPlayerController::RunSelfTest, 0.5f, false);
	}
}

void AEOPlayerController::HandleMissionStateChanged(EEOMissionState OldState, EEOMissionState NewState)
{
	// Complete and Failed are terminal: nothing transitions out of them, and the
	// deployment gate refuses anything that is not Inactive or InFlight. Without
	// this the player finishes one run and the deploy key is dead until they open
	// the console.
	if (NewState == EEOMissionState::Complete || NewState == EEOMissionState::Failed)
	{
		GetWorldTimerManager().SetTimer(
			RearmTimer, this, &AEOPlayerController::RearmMission, RearmDelay, false);
	}
}

void AEOPlayerController::RearmMission()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// The objective is the one piece of mission state that is not owned by the
	// subsystem, so re-arming has to reach into it explicitly. Left set, the
	// second run has a terminal that can never be interacted with and an
	// extraction that can never arm.
	for (TActorIterator<AEOObjectiveTerminal> It(World); It; ++It)
	{
		It->ResetObjective();
	}

	for (TActorIterator<AEOGuardCharacter> It(World); It; ++It)
	{
		It->ResetGuard();
	}

	if (Operative)
	{
		Operative->ResetOperative();
		IEODeployableInterface::Execute_SetStowed(Operative, true);
	}

	if (UEOMissionSubsystem* Mission = World->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->ResetMission();
	}

	UE_LOG(LogExecutiveOps, Log, TEXT("Mission re-armed; ready for another run."));
}

void AEOPlayerController::RunSelfTest()
{
	// The test object owns its own phase timer and reports its own result.
	SelfTest = NewObject<UEOSelfTest>(this, TEXT("EOSelfTest"));
	SelfTest->Start(this);
}

AEOAircraftPawn* AEOPlayerController::ResolveAircraft()
{
	if (Aircraft)
	{
		return Aircraft;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AEOAircraftPawn> It(World); It; ++It)
	{
		Aircraft = *It;
		break;
	}

	if (!Aircraft && AircraftClass)
	{
		// No placed aircraft: spawn one at the player start so the map still boots.
		const FTransform SpawnAt = GetPawn() ? GetPawn()->GetActorTransform() : FTransform::Identity;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Aircraft = World->SpawnActor<AEOAircraftPawn>(AircraftClass, SpawnAt, Params);
		UE_LOG(LogExecutiveOps, Log, TEXT("No aircraft placed in level; spawned %s."), *GetNameSafe(Aircraft));
	}

	if (Aircraft)
	{
		AircraftStartTransform = Aircraft->GetActorTransform();
	}
	return Aircraft;
}

AEOOperativeCharacter* AEOPlayerController::ResolveOperative()
{
	if (Operative)
	{
		return Operative;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AEOOperativeCharacter> It(World); It; ++It)
	{
		Operative = *It;
		break;
	}

	if (!Operative && OperativeClass)
	{
		FTransform SpawnAt = FTransform::Identity;
		if (Aircraft)
		{
			SpawnAt = IEOAircraftControlInterface::Execute_GetDeploymentSocketTransform(Aircraft);
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Operative = World->SpawnActor<AEOOperativeCharacter>(OperativeClass, SpawnAt, Params);
		UE_LOG(LogExecutiveOps, Log, TEXT("No operative placed in level; spawned %s."), *GetNameSafe(Operative));
	}

	if (Operative)
	{
		OperativeStartTransform = Operative->GetActorTransform();
	}
	return Operative;
}

void AEOPlayerController::ApplyMappingContext(EEOControlMode Mode)
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem || !InputConfig)
	{
		return;
	}

	// Exactly one context is ever active, so aircraft and ground bindings cannot collide.
	Subsystem->ClearAllMappings();

	switch (Mode)
	{
	case EEOControlMode::Aircraft:
		Subsystem->AddMappingContext(InputConfig->AircraftContext, 0);
		break;
	case EEOControlMode::Operative:
		Subsystem->AddMappingContext(InputConfig->OperativeContext, 0);
		break;
	default:
		break;
	}
}

bool AEOPlayerController::PossessAircraft()
{
	if (!ResolveAircraft())
	{
		return false;
	}

	Possess(Aircraft);

	// Returning to the aircraft always releases the deployment freeze. Leaving it
	// set strands the player in a craft that silently ignores every input.
	IEOAircraftControlInterface::Execute_SetDeploymentHold(Aircraft, false);

	// A drop still in progress is abandoned rather than left running: otherwise
	// the operative keeps falling and drives the mission to OnGround while the
	// player is flying.
	if (Operative)
	{
		Operative->CancelDeploymentDrop();
		IEODeployableInterface::Execute_SetStowed(Operative, true);
	}

	ControlMode = EEOControlMode::Aircraft;
	ApplyMappingContext(ControlMode);
	UE_LOG(LogExecutiveOps, Log, TEXT("Control mode: Aircraft."));
	return true;
}

bool AEOPlayerController::PossessOperative()
{
	if (!ResolveOperative())
	{
		return false;
	}

	Possess(Operative);
	ControlMode = EEOControlMode::Operative;
	ApplyMappingContext(ControlMode);
	UE_LOG(LogExecutiveOps, Log, TEXT("Control mode: Operative."));
	return true;
}

AEOMissionSite* AEOPlayerController::GetSelectedSite() const
{
	const UEOMissionSubsystem* Mission = GetWorld()
		? GetWorld()->GetSubsystem<UEOMissionSubsystem>() : nullptr;
	return Mission ? Mission->GetSelectedSite() : nullptr;
}

FString AEOPlayerController::GetDeploymentBlocker() const
{
	if (ControlMode != EEOControlMode::Aircraft || !Aircraft)
	{
		return TEXT("not flying");
	}

	if (Operative && Operative->IsDeploying())
	{
		return TEXT("already deploying");
	}

	// A drop with no site would throw the operative into empty city. Selection is
	// a precondition, not an optional refinement.
	const AEOMissionSite* Site = GetSelectedSite();
	if (!Site)
	{
		return TEXT("no mission selected");
	}

	if (!Site->IsWithinHoverVolume(Aircraft))
	{
		return TEXT("outside the deployment zone");
	}

	if (!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Aircraft))
	{
		return TEXT("hold hover and slow down");
	}

	// The mission state machine is the real authority on whether a drop is legal;
	// without this the HUD would read READY while the deploy key did nothing.
	const UEOMissionSubsystem* Mission = GetWorld()
		? GetWorld()->GetSubsystem<UEOMissionSubsystem>() : nullptr;
	if (Mission
		&& Mission->GetMissionState() != EEOMissionState::Inactive
		&& Mission->GetMissionState() != EEOMissionState::InFlight)
	{
		return TEXT("mission already underway");
	}

	return FString();
}

bool AEOPlayerController::CanDeploy() const
{
	return GetDeploymentBlocker().IsEmpty();
}

void AEOPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDeploymentAssist();
}

void AEOPlayerController::UpdateDeploymentAssist()
{
	if (ControlMode != EEOControlMode::Aircraft || !Aircraft)
	{
		return;
	}

	const AEOMissionSite* Site = GetSelectedSite();

	// The craft only helps once the player has brought it into the zone and
	// committed to a hover. Outside that, it stays entirely in their hands.
	const bool bAssist = Site
		&& Site->IsWithinHoverVolume(Aircraft)
		&& IEOAircraftControlInterface::Execute_IsHovering(Aircraft);

	IEOAircraftControlInterface::Execute_SetStationKeepTarget(
		Aircraft, Site ? Site->GetHoverPoint() : FVector::ZeroVector, bAssist);
}

bool AEOPlayerController::RequestDeployment()
{
	if (!CanDeploy())
	{
		UE_LOG(LogExecutiveOps, Log, TEXT("Deployment refused: %s."), *GetDeploymentBlocker());
		return false;
	}

	if (!ResolveOperative())
	{
		UE_LOG(LogExecutiveOps, Error, TEXT("Deployment refused: no operative available."));
		return false;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	if (Mission)
	{
		// M2 has no separate commit step outside the map, so start implicitly.
		Mission->StartMission();
		if (!Mission->BeginDeployment())
		{
			return false;
		}
	}

	// The craft parks itself for the drop. It is frozen, not flying itself.
	IEOAircraftControlInterface::Execute_SetStationKeepTarget(Aircraft, FVector::ZeroVector, false);
	IEOAircraftControlInterface::Execute_SetDeploymentHold(Aircraft, true);

	const FTransform Socket = IEOAircraftControlInterface::Execute_GetDeploymentSocketTransform(Aircraft);
	IEODeployableInterface::Execute_OnDeployFrom(Operative, Aircraft, Socket);

	// Thrown clear, down and slightly forward, rather than simply released.
	const FVector Launch =
		-Aircraft->GetActorUpVector() * DeployLaunchDown +
		Aircraft->GetActorForwardVector() * DeployLaunchForward;
	Operative->BeginDeploymentDrop(Socket, Launch);

	// Possess immediately: the player rides the drop down, which is what makes
	// this read as one continuous move instead of a cut between two games.
	// Ground input stays locked until the operative lands.
	if (!PossessOperative())
	{
		// The world is already mid-deployment; unwind it rather than leaving the
		// player in a frozen aircraft with an operative falling beside them.
		UE_LOG(LogExecutiveOps, Error, TEXT("Deployment failed at possession; rolling back."));
		AbortDeployment();
		return false;
	}

	return true;
}

void AEOPlayerController::AbortDeployment()
{
	if (Operative)
	{
		Operative->CancelDeploymentDrop();
		IEODeployableInterface::Execute_SetStowed(Operative, true);
	}

	if (Aircraft)
	{
		IEOAircraftControlInterface::Execute_SetDeploymentHold(Aircraft, false);
	}

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->AbortDeployment();
	}

	PossessAircraft();
}

bool AEOPlayerController::RequestExtraction()
{
	if (ControlMode != EEOControlMode::Operative || !Operative)
	{
		return false;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();

	// An extraction called with the objective done closes the mission out. Called
	// at any other time it is just the debug handover back to the aircraft, which
	// the self-test and the cheat manager both rely on.
	const bool bCompletesMission =
		Mission && Mission->GetMissionState() == EEOMissionState::ObjectiveComplete;

	IEODeployableInterface::Execute_OnExtractBegin(Operative, Aircraft);

	// Possession first. Committing the state machine to Extracting and then
	// failing to possess would strand the mission in a state nothing transitions
	// out of, with the player still standing on the ground.
	if (!PossessAircraft())
	{
		return false;
	}

	if (bCompletesMission)
	{
		// M7 replaces this instant hand-back with the aircraft actually arriving.
		Mission->BeginExtraction();
		Mission->CompleteMission();
		UE_LOG(LogExecutiveOps, Log, TEXT("Mission complete."));
	}

	return true;
}

void AEOPlayerController::EOReset()
{
	if (Aircraft)
	{
		Aircraft->SetActorTransform(AircraftStartTransform);
		IEOAircraftControlInterface::Execute_ResetFlightState(Aircraft);
		IEOAircraftControlInterface::Execute_SetDeploymentHold(Aircraft, false);
	}

	if (Operative)
	{
		// Cancel before moving: a live drop or traversal would otherwise keep
		// running and drag the operative back across the level, or fire
		// CompleteDeployment against a mission that just reset.
		Operative->CancelDeploymentDrop();

		// Revives and restores movement. Without it a player killed by the guard
		// resets into a corpse they still control and can never play again.
		Operative->ResetOperative();
		Operative->SetActorTransform(OperativeStartTransform);
	}

	// Guards come back too, or the second run of a mission has nothing in it.
	for (TActorIterator<AEOGuardCharacter> It(GetWorld()); It; ++It)
	{
		It->ResetGuard();
	}

	for (TActorIterator<AEOObjectiveTerminal> It(GetWorld()); It; ++It)
	{
		It->ResetObjective();
	}

	GetWorldTimerManager().ClearTimer(RearmTimer);

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->ResetMission();
	}

	PossessAircraft();
	UE_LOG(LogExecutiveOps, Log, TEXT("Reset to boot state."));
}
