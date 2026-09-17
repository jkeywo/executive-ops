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
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
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

	// Pick up the district's mission on boot.
	//
	// Selection used to be console-only, which left the player flying a city with
	// no waypoint, no marker and a deploy key that answered "no mission selected"
	// forever - there was literally nowhere to go. With one site in the
	// prototype, having it already selected is also simply the right default;
	// a selection screen is a later milestone's problem.
	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		if (const AEOMissionSite* Site = Mission->SelectDefaultSite())
		{
			UE_LOG(LogExecutiveOps, Log, TEXT("Mission available: %s"),
				*Site->GetDisplayName().ToString());
		}
	}

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

	if (FParse::Param(FCommandLine::Get(), TEXT("EOScreenshot")))
	{
		float Delay = 4.f;
		FParse::Value(FCommandLine::Get(), TEXT("EOScreenshotDelay="), Delay);

		FTimerHandle ShotTimer;
		GetWorldTimerManager().SetTimer(
			ShotTimer, this, &AEOPlayerController::TakeDebugScreenshot, Delay, false);
	}

	if (UEOSelfTest::IsRequested())
	{
		// Deferred a tick: the transitions need a fully possessed pawn.
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AEOPlayerController::RunSelfTest, 0.5f, false);
	}
}

void AEOPlayerController::TakeDebugScreenshot()
{
	// -EOScreenshotCockpit captures from the first-person seat, which is the only
	// way to check that a cockpit interior is facing the right way.
	if (FParse::Param(FCommandLine::Get(), TEXT("EOScreenshotCockpit")))
	{
		if (AEOAircraftPawn* Craft = Cast<AEOAircraftPawn>(GetPawn()))
		{
			Craft->SetFirstPerson(true);
		}
	}

	// -EOScreenshotOrbit=<yaw> views the possessed pawn from a bearing instead of
	// down the chase camera, which always sits behind and so can never show
	// whether a model is facing the right way.
	float OrbitYaw = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("EOScreenshotOrbit="), OrbitYaw))
	{
		// An orbit shot exists to look at the exterior, and the aircraft boots
		// into the cockpit with the hull hidden - so leave first person first,
		// or the shot shows an interior floating in mid-air.
		if (AEOAircraftPawn* Craft = Cast<AEOAircraftPawn>(GetPawn()))
		{
			Craft->SetFirstPerson(false);
		}

		if (APawn* Subject = GetPawn())
		{
			float OrbitDistance = 1600.f;
			FParse::Value(FCommandLine::Get(), TEXT("EOScreenshotDistance="), OrbitDistance);

			float OrbitPitch = -10.f;
			FParse::Value(FCommandLine::Get(), TEXT("EOScreenshotPitch="), OrbitPitch);

			// Bearing is relative to the pawn's own facing, so 90 is always its
			// left side whichever way it happens to be pointing.
			const FRotator Bearing(0.f, Subject->GetActorRotation().Yaw + OrbitYaw, 0.f);
			const FVector Offset = Bearing.Vector() * OrbitDistance + FVector(0.f, 0.f, 250.f);
			const FVector ViewLocation = Subject->GetActorLocation() + Offset;

			if (ACameraActor* ViewCamera = GetWorld()->SpawnActor<ACameraActor>(
				ViewLocation, (Subject->GetActorLocation() - ViewLocation).Rotation()))
			{
				ViewCamera->SetActorRotation(
					FRotator(OrbitPitch, (Subject->GetActorLocation() - ViewLocation).Rotation().Yaw, 0.f));
				SetViewTarget(ViewCamera);
			}
		}
	}

	// Let the view settle before capturing. Cutting to a new camera and grabbing
	// the same frame smears the shot with motion blur from the jump.
	FTimerHandle CaptureTimer;
	GetWorldTimerManager().SetTimer(
		CaptureTimer, this, &AEOPlayerController::CaptureDebugScreenshot, 0.75f, false);
}

void AEOPlayerController::CaptureDebugScreenshot()
{
	FScreenshotRequest::RequestScreenshot(TEXT("EOShot"), /*bShowUI=*/true, /*bAddFilenameSuffix=*/false);
	UE_LOG(LogExecutiveOps, Display, TEXT("Screenshot requested."));

	// Give the request a frame or two to land before tearing the game down.
	FTimerHandle QuitTimer;
	GetWorldTimerManager().SetTimer(QuitTimer, FTimerDelegate::CreateLambda([]()
	{
		FPlatformMisc::RequestExit(false);
	}), 2.f, false);
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

	// A rearm timer left over from the previous run must not reset the mission
	// while the aircraft is on its way in to collect the player.
	if (bExtractionInbound)
	{
		GetWorldTimerManager().SetTimer(
			RearmTimer, this, &AEOPlayerController::RearmMission, RearmDelay, false);
		return;
	}

	// Only re-arm a mission that is still closed out. If something else has
	// already moved it on, this timer is stale and firing it would tear down a
	// run that is currently underway.
	UEOMissionSubsystem* Mission = World->GetSubsystem<UEOMissionSubsystem>();
	if (Mission
		&& Mission->GetMissionState() != EEOMissionState::Complete
		&& Mission->GetMissionState() != EEOMissionState::Failed)
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

	bExtractionInbound = false;

	if (Mission)
	{
		Mission->ResetMission();
	}

	// The operative was just stowed - hidden and collisionless. Without this the
	// player is left driving an invisible character with the ground bindings and
	// no way back to the aircraft.
	PossessAircraft();

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

	// Returning to the aircraft always releases the deployment freeze and any
	// scripted arrival. Leaving either set strands the player in a craft that
	// silently ignores every input.
	IEOAircraftControlInterface::Execute_SetDeploymentHold(Aircraft, false);
	IEOAircraftControlInterface::Execute_SetScriptedDestination(Aircraft, FVector::ZeroVector, false);
	bExtractionInbound = false;

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
		// Say where, not just that it is wrong. "Outside the zone" is no help
		// when the player cannot tell which way the zone is.
		const float DistanceM =
			FVector::Dist(Aircraft->GetActorLocation(), Site->GetHoverPoint()) * 0.01f;
		return FString::Printf(TEXT("fly to the marker (%.0fm)"), DistanceM);
	}

	if (!IEOAircraftControlInterface::Execute_IsHovering(Aircraft))
	{
		return TEXT("hold hover to steady the craft");
	}

	if (!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Aircraft))
	{
		return TEXT("slow down to deploy");
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
	UpdateExtraction(DeltaSeconds);
	UpdateScreenFlash(DeltaSeconds);
}

void AEOPlayerController::TriggerScreenFlash(const FLinearColor& Colour,
	float HoldSeconds, float FadeSeconds)
{
	FlashColour = Colour;
	FlashAlpha = 1.f;
	FlashHoldRemaining = FMath::Max(HoldSeconds, 0.f);
	FlashFadeSeconds = FMath::Max(FadeSeconds, KINDA_SMALL_NUMBER);
}

void AEOPlayerController::UpdateScreenFlash(float DeltaSeconds)
{
	if (FlashAlpha <= 0.f)
	{
		return;
	}

	// Opaque for the hold, then a linear fade. Linear rather than eased because
	// the point is to stop covering the screen promptly once the cut is hidden.
	if (FlashHoldRemaining > 0.f)
	{
		FlashHoldRemaining -= DeltaSeconds;
		return;
	}

	FlashAlpha = FMath::Max(FlashAlpha - DeltaSeconds / FlashFadeSeconds, 0.f);
}

float AEOPlayerController::GetExtractionDistance() const
{
	if (!bExtractionInbound || !Aircraft || !Operative)
	{
		return -1.f;
	}
	return FVector::Dist(Aircraft->GetActorLocation(), Operative->GetActorLocation()) * 0.01f;
}

bool AEOPlayerController::BeginExtractionPickup()
{
	if (!Aircraft || !Operative)
	{
		return false;
	}

	// Called in over the operative rather than to the pad: by the time the craft
	// arrives the player may have been driven off the pad, and being collected
	// where you actually are is the release the sequence is meant to be.
	ExtractionPoint = Operative->GetActorLocation() + FVector(0.f, 0.f, ExtractionHoverHeight);
	ExtractionElapsed = 0.f;
	PickupElapsed = 0.f;
	bExtractionInbound = true;

	IEOAircraftControlInterface::Execute_SetStationKeepTarget(Aircraft, FVector::ZeroVector, false);
	IEOAircraftControlInterface::Execute_SetScriptedDestination(Aircraft, ExtractionPoint, true);

	UE_LOG(LogExecutiveOps, Log, TEXT("Extraction called; aircraft inbound."));
	return true;
}

void AEOPlayerController::CancelExtraction()
{
	if (Aircraft)
	{
		// Leaving the craft flying itself means the player takes over an aircraft
		// that silently ignores every input.
		IEOAircraftControlInterface::Execute_SetScriptedDestination(Aircraft, FVector::ZeroVector, false);
	}

	bExtractionInbound = false;
	ExtractionElapsed = 0.f;
	PickupElapsed = 0.f;
}

void AEOPlayerController::UpdateExtraction(float DeltaSeconds)
{
	if (!bExtractionInbound)
	{
		return;
	}

	if (!Aircraft || !Operative)
	{
		CancelExtraction();
		return;
	}

	// A player killed while the craft is inbound is not being collected. The
	// mission has already failed and will re-arm on its own.
	if (Operative->IsDead())
	{
		UE_LOG(LogExecutiveOps, Log, TEXT("Extraction aborted: operative died."));
		CancelExtraction();
		return;
	}

	ExtractionElapsed += DeltaSeconds;

	// Read the arrival before refreshing the destination, or the refresh below
	// invalidates the very flag being tested.
	const bool bOverhead = IEOAircraftControlInterface::Execute_HasReachedScriptedDestination(Aircraft);

	// Keep the destination over the operative while they are still moving, so a
	// player fighting their way to the pad is still collected.
	if (!bOverhead && PickupElapsed <= 0.f)
	{
		ExtractionPoint = Operative->GetActorLocation() + FVector(0.f, 0.f, ExtractionHoverHeight);
		IEOAircraftControlInterface::Execute_SetScriptedDestination(Aircraft, ExtractionPoint, true);
	}

	if (bOverhead)
	{
		// Short pickup beat once the craft is overhead, so boarding reads as an
		// action rather than a teleport.
		PickupElapsed += DeltaSeconds;
		if (PickupElapsed >= PickupDuration)
		{
			CompleteExtractionPickup();
		}
		return;
	}

	if (ExtractionElapsed >= ExtractionTimeout)
	{
		// Arrival blocked - scenery, a bad approach line. Collect the player
		// anyway rather than leaving them standing in a finished mission.
		UE_LOG(LogExecutiveOps, Warning,
			TEXT("Extraction arrival timed out after %.0fs; collecting anyway."), ExtractionElapsed);
		CompleteExtractionPickup();
	}
}

void AEOPlayerController::CompleteExtractionPickup()
{
	bExtractionInbound = false;

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();

	if (Operative)
	{
		IEODeployableInterface::Execute_OnExtractBegin(Operative, Aircraft);
	}

	if (Aircraft)
	{
		IEOAircraftControlInterface::Execute_SetScriptedDestination(Aircraft, FVector::ZeroVector, false);
	}

	if (!PossessAircraft())
	{
		// Extracting has no transition out, so leaving the mission there would
		// end the run with no way to recover. Put it back to a playable state.
		UE_LOG(LogExecutiveOps, Error, TEXT("Extraction pickup could not possess the aircraft."));
		if (Mission)
		{
			Mission->ResetMission();
		}
		return;
	}

	if (Mission && Mission->GetMissionState() == EEOMissionState::Extracting)
	{
		Mission->CompleteMission();
		UE_LOG(LogExecutiveOps, Log, TEXT("Mission complete; flying away."));
	}
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

	// Before the swap, not after: the pawn, camera and HUD all change on the next
	// line, and the wash has to already be opaque to cover it.
	TriggerScreenFlash(DeployFlashColour, DeployFlashHold, DeployFlashFade);

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

	if (bCompletesMission)
	{
		// The real extraction: call the craft in, keep the operative playable
		// while it comes, and hand control over when it arrives.
		if (bExtractionInbound)
		{
			return false;
		}

		if (!Mission->BeginExtraction())
		{
			return false;
		}

		if (!BeginExtractionPickup())
		{
			// Nothing to fly in. Fall back to the immediate handover rather than
			// stranding the mission in Extracting.
			IEODeployableInterface::Execute_OnExtractBegin(Operative, Aircraft);
			if (!PossessAircraft())
			{
				Mission->ResetMission();
				return false;
			}
			Mission->CompleteMission();
		}
		return true;
	}

	// Debug handover, used by the cheat manager and the self-test. Cancel any
	// arrival first, or the pickup fires again later and the player spends the
	// interim in an aircraft that ignores input.
	CancelExtraction();
	IEODeployableInterface::Execute_OnExtractBegin(Operative, Aircraft);
	return PossessAircraft();
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
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AEOGuardCharacter> It(World); It; ++It)
		{
			It->ResetGuard();
		}

		for (TActorIterator<AEOObjectiveTerminal> It(World); It; ++It)
		{
			It->ResetObjective();
		}
	}

	GetWorldTimerManager().ClearTimer(RearmTimer);

	bExtractionInbound = false;
	ExtractionElapsed = 0.f;
	PickupElapsed = 0.f;

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->ResetMission();
	}

	PossessAircraft();
	UE_LOG(LogExecutiveOps, Log, TEXT("Reset to boot state."));
}
