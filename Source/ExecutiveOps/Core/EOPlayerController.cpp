#include "Core/EOPlayerController.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "ExecutiveOps.h"
#include "Debug/EOCheatManager.h"
#include "Debug/EOSelfTest.h"
#include "Input/EOInputConfig.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EODeployableInterface.h"
#include "Mission/EOMissionSubsystem.h"

AEOPlayerController::AEOPlayerController()
{
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

	if (UEOSelfTest::IsRequested())
	{
		// Deferred a tick: the transitions need a fully possessed pawn.
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AEOPlayerController::RunSelfTest, 0.5f, false);
	}
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

	// The operative rides along invisibly rather than standing in the world.
	if (Operative)
	{
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

bool AEOPlayerController::RequestDeployment()
{
	if (ControlMode != EEOControlMode::Aircraft || !Aircraft)
	{
		return false;
	}

	if (!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Aircraft))
	{
		UE_LOG(LogExecutiveOps, Log, TEXT("Deployment refused: aircraft not stabilised (hold hover, slow down)."));
		return false;
	}

	if (!ResolveOperative())
	{
		return false;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	if (Mission)
	{
		// M0 has no mission selection step, so start one implicitly if needed.
		Mission->StartMission();
		Mission->BeginDeployment();
	}

	const FTransform Socket = IEOAircraftControlInterface::Execute_GetDeploymentSocketTransform(Aircraft);
	IEODeployableInterface::Execute_OnDeployFrom(Operative, Aircraft, Socket);

	if (!PossessOperative())
	{
		return false;
	}

	IEODeployableInterface::Execute_OnDeployComplete(Operative);

	if (Mission)
	{
		Mission->CompleteDeployment();
	}
	return true;
}

bool AEOPlayerController::RequestExtraction()
{
	if (ControlMode != EEOControlMode::Operative || !Operative)
	{
		return false;
	}

	IEODeployableInterface::Execute_OnExtractBegin(Operative, Aircraft);

	if (!PossessAircraft())
	{
		return false;
	}

	// M7 drives the real sequence through the mission subsystem; M0 just returns control.
	UE_LOG(LogExecutiveOps, Log, TEXT("Extraction placeholder: returned to aircraft control."));
	return true;
}

void AEOPlayerController::EOReset()
{
	if (Aircraft)
	{
		Aircraft->SetActorTransform(AircraftStartTransform);
		IEOAircraftControlInterface::Execute_ResetFlightState(Aircraft);
	}

	if (Operative)
	{
		Operative->SetActorTransform(OperativeStartTransform);
		Operative->GetCharacterMovement()->StopMovementImmediately();
	}

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->ResetMission();
	}

	PossessAircraft();
	UE_LOG(LogExecutiveOps, Log, TEXT("Reset to boot state."));
}
