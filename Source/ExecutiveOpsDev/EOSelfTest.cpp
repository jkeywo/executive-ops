#include "EOSelfTest.h"

#include "ExecutiveOpsDev.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Combat/EOWeaponComponent.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/EODeployableInterface.h"
#include "Core/EOPlayerController.h"
#include "ExecutiveOps.h"
#include "GameFramework/Character.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Mission/EOExtractionZone.h"
#include "Mission/EOInteractableInterface.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOObjectiveTerminal.h"
#include "Mission/EOMissionSubsystem.h"
#include "TimerManager.h"

namespace
{
	/** How often the phase machine is serviced. Fine enough to sample smoothly. */
	constexpr float StepInterval = 0.05f;
}

bool UEOSelfTest::IsRequested()
{
	return FParse::Param(FCommandLine::Get(), TEXT("EOSelfTest"));
}

bool UEOSelfTest::ShouldExitAfterRun()
{
	return FParse::Param(FCommandLine::Get(), TEXT("EOSelfTestExit"));
}

bool UEOSelfTest::IsGroundTest()
{
	return FParse::Param(FCommandLine::Get(), TEXT("EOGroundTest"));
}

AEOOperativeCharacter* UEOSelfTest::GetOperative() const
{
	return Controller ? Cast<AEOOperativeCharacter>(Controller->GetPawn()) : nullptr;
}

APawn* UEOSelfTest::FindAircraftInLevel() const
{
	if (!Controller || !Controller->GetWorld())
	{
		return nullptr;
	}
	for (TActorIterator<AEOAircraftPawn> It(Controller->GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AEOGuardCharacter* UEOSelfTest::GetGuard() const
{
	if (!Controller || !Controller->GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<AEOGuardCharacter> It(Controller->GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AEOObjectiveTerminal* UEOSelfTest::GetObjective() const
{
	if (!Controller || !Controller->GetWorld())
	{
		return nullptr;
	}
	for (TActorIterator<AEOObjectiveTerminal> It(Controller->GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AEOExtractionZone* UEOSelfTest::GetExtractionZone() const
{
	if (!Controller || !Controller->GetWorld())
	{
		return nullptr;
	}
	for (TActorIterator<AEOExtractionZone> It(Controller->GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

bool UEOSelfTest::EnterGroundMission()
{
	UEOMissionSubsystem* Mission = Controller
		? Controller->GetWorld()->GetSubsystem<UEOMissionSubsystem>() : nullptr;
	if (!Mission)
	{
		return false;
	}

	// Stands in for the flight and the drop, which the flight suite covers.
	Mission->ResetMission();
	return Mission->StartMission()
		&& Mission->BeginDeployment()
		&& Mission->CompleteDeployment();
}

void UEOSelfTest::ResetEncounter()
{
	// Every outcome is tested against the SAME encounter, so each one starts from
	// an identical, unaware state rather than inheriting the last test's mess.
	if (AEOGuardCharacter* Guard = GetGuard())
	{
		Guard->ResetGuard();
	}

	if (AEOOperativeCharacter* Op = GetOperative())
	{
		if (UEOHealthComponent* OpHealth = Op->GetHealth())
		{
			OpHealth->Revive();
		}
	}
}

void UEOSelfTest::Check(bool bCondition, const FString& Description)
{
	++Checks;
	if (bCondition)
	{
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest]   PASS  %s"), *Description);
	}
	else
	{
		++Failures;
		UE_LOG(LogExecutiveOpsDev, Error, TEXT("[SelfTest]   FAIL  %s"), *Description);
	}
}

APawn* UEOSelfTest::GetAircraftPawn() const
{
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return (Pawn && Pawn->Implements<UEOAircraftControlInterface>()) ? Pawn : nullptr;
}

void UEOSelfTest::Start(AEOPlayerController* InController)
{
	Controller = InController;
	if (!Controller)
	{
		UE_LOG(LogExecutiveOpsDev, Error, TEXT("[SelfTest] no player controller"));
		return;
	}

	UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] === self-test starting ==="));

	Phase = IsGroundTest() ? EPhase::GroundSetup : EPhase::CoreState;
	PhaseDwell = 0.f;
	PhaseElapsed = 0.f;

	Controller->GetWorldTimerManager().SetTimer(
		StepTimer, this, &UEOSelfTest::Step, StepInterval, /*bLoop=*/true);
}

void UEOSelfTest::Advance(EPhase Next, float DwellSeconds)
{
	Phase = Next;
	PhaseDwell = DwellSeconds;
	PhaseElapsed = 0.f;
}

void UEOSelfTest::Step()
{
	if (!Controller)
	{
		Finish();
		return;
	}

	// Some phases need to act continuously while their dwell runs down.
	if (Phase == EPhase::NavigateToSite)
	{
		SteerTowardSite();
	}

	// Let a phase settle for its dwell before its checks are sampled.
	PhaseElapsed += StepInterval;
	if (PhaseElapsed < PhaseDwell)
	{
		return;
	}

	UEOMissionSubsystem* Mission = Controller->GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	APawn* Craft = GetAircraftPawn();

	switch (Phase)
	{
	// ---- M0: boots into a controllable aircraft ------------------------------
	case EPhase::CoreState:
	{
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M0: boot and control --"));
		Check(Mission != nullptr, TEXT("mission subsystem exists"));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("boots possessing the aircraft"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Inactive,
			TEXT("boots with mission Inactive"));
		Check(Craft != nullptr, TEXT("possessed pawn implements the aircraft interface"));
		BootPawn = Craft;

		Check(Mission && Mission->SelectDefaultSite() != nullptr,
			TEXT("the level provides a mission site"));
		Advance(EPhase::DeployGuard, 0.f);
		break;
	}

	case EPhase::DeployGuard:
	{
		if (Craft)
		{
			// Far from the site: the zone is the blocker, whatever the craft does.
			IEOAircraftControlInterface::Execute_SetHoverEnabled(Craft, true);
			// Asserts the meaning, not the exact wording: the prompt tells the
			// player to fly to the marker and how far it is.
			const FString Blocker = Controller->GetDeploymentBlocker();
			Check(Blocker.Contains(TEXT("marker")),
				FString::Printf(TEXT("deployment blocked by distance (got '%s')"), *Blocker));
			Check(!Controller->CanDeploy(), TEXT("cannot deploy from outside the zone"));
			Check(!Controller->RequestDeployment(), TEXT("deployment request refused outside the zone"));

			IEOAircraftControlInterface::Execute_SetHoverEnabled(Craft, false);
			Check(!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("not ready to deploy while not hovering"));
		}
		Advance(EPhase::Reset, 0.f);
		break;
	}

	case EPhase::Reset:
	{
		Controller->EOReset();
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("reset leaves the player in the aircraft"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Inactive,
			TEXT("reset returns mission to Inactive"));
		Advance(EPhase::IllegalTransitions, 0.f);
		break;
	}

	case EPhase::IllegalTransitions:
	{
		if (Mission)
		{
			Check(!Mission->CompleteMission(), TEXT("cannot complete a mission that never started"));
			Check(!Mission->BeginExtraction(), TEXT("cannot extract before the objective is done"));
			Check(Mission->GetMissionState() == EEOMissionState::Inactive,
				TEXT("rejected transitions leave state untouched"));
		}

		// Hand over to the flight checks with a known-still craft.
		if (Craft)
		{
			IEOAircraftControlInterface::Execute_ResetFlightState(Craft);
			IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector(1.f, 0.f, 0.f));
		}
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M1: flight --"));
		Advance(EPhase::FlightAccelerate, 1.5f);
		break;
	}

	// ---- M1: the craft actually flies ----------------------------------------
	case EPhase::FlightAccelerate:
	{
		if (Craft)
		{
			SpeedSample = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Craft);
			Check(SpeedSample > 500.f,
				FString::Printf(TEXT("accelerates under sustained input (%.0f cm/s)"), SpeedSample));
			Check(!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("cannot deploy while moving fast"));
		}
		// Actually release the throttle, then check momentum bleeds off.
		if (Craft)
		{
			IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector::ZeroVector);
		}
		Advance(EPhase::FlightCoast, 2.0f);
		break;
	}

	case EPhase::FlightCoast:
	{
		if (Craft)
		{
			const float Coasted = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Craft);
			Check(Coasted < SpeedSample,
				FString::Printf(TEXT("decelerates once input is released (%.0f -> %.0f cm/s)"),
					SpeedSample, Coasted));
		}

		// Drive back up to speed, then demand hover, to prove the clamp applies.
		if (Craft)
		{
			IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector(1.f, 0.f, 0.f));
		}
		Advance(EPhase::HoverEngage, 1.5f);
		break;
	}

	case EPhase::HoverEngage:
	{
		if (AEOAircraftPawn* Plane = Cast<AEOAircraftPawn>(Craft))
		{
			SpeedSample = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane);
			Check(Plane->GetHoverBlend() <= KINDA_SMALL_NUMBER,
				TEXT("hover blend is zero while flying"));
			Check(Plane->GetThrustAlpha() > 0.5f, TEXT("thrust reads high under input"));

			IEOAircraftControlInterface::Execute_SetHoverEnabled(Plane, true);
		}
		Advance(EPhase::HoverSpeedClamp, 1.5f);
		break;
	}

	case EPhase::HoverSpeedClamp:
	{
		if (AEOAircraftPawn* Plane = Cast<AEOAircraftPawn>(Craft))
		{
			Check(Plane->GetHoverBlend() >= 0.99f,
				FString::Printf(TEXT("hover blend reaches 1 (%.2f)"), Plane->GetHoverBlend()));

			const float Hovering = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane);
			Check(Hovering < SpeedSample,
				FString::Printf(TEXT("engaging hover sheds speed (%.0f -> %.0f cm/s)"),
					SpeedSample, Hovering));

			// Entering hover at speed must not leave the craft above the hover cap.
			Check(Hovering <= 1000.f,
				FString::Printf(TEXT("hover clamps speed to the hover maximum (%.0f cm/s)"), Hovering));

			IEOAircraftControlInterface::Execute_ResetFlightState(Plane);
			Check(FMath::IsNearlyZero(IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane)),
				TEXT("reset clears velocity"));

			// The cockpit panel follows the view. Possession already lands in first
			// person, so the switch is the part nothing else here exercised: chase
			// takes the panel off the glass and coming back puts it there again.
			// Whichever of the two panels is live is the pawn's own affair.
			Check(Plane->IsFirstPerson(), TEXT("the craft is possessed in first person"));
			Check(Plane->IsCockpitPanelShowing(), TEXT("and the cockpit panel is showing"));

			Plane->SetFirstPerson(false);
			Check(!Plane->IsCockpitPanelShowing(), TEXT("chase view takes the panel off the glass"));

			Plane->SetFirstPerson(true);
			Check(Plane->IsCockpitPanelShowing(), TEXT("first person puts it back"));
		}
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M2: navigation --"));
		Advance(EPhase::MissionSelect, 0.f);
		break;
	}

	// ---- M2: select a site and fly the approach -------------------------------
	case EPhase::MissionSelect:
	{
		AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;
		Check(Site != nullptr, TEXT("selected site survives a reset"));

		if (Site && Craft)
		{
			Check(!Site->IsWithinHoverVolume(Craft),
				TEXT("aircraft does not start inside the hover volume"));
			Check(Site->GetHoverRadius() > 0.f, TEXT("hover volume has a radius"));

			DistanceSample = FVector::Dist(Craft->GetActorLocation(), Site->GetHoverPoint());
			Check(DistanceSample > 15000.f,
				FString::Printf(TEXT("site is a real flight away (%.0fm)"), DistanceSample * 0.01f));
		}

		Check(Mission && Mission->StartMission(), TEXT("mission starts"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::InFlight,
			TEXT("starting the mission puts it In Flight"));

		// Retargeting mid-approach would leave the ground phase pointing elsewhere.
		Advance(EPhase::NavigateToSite, 32.f);
		break;
	}

	case EPhase::NavigateToSite:
	{
		AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;
		if (Site && Craft)
		{
			const float Remaining = FVector::Dist(Craft->GetActorLocation(), Site->GetHoverPoint());
			Check(Remaining < DistanceSample,
				FString::Printf(TEXT("flying the approach closes the distance (%.0fm -> %.0fm)"),
					DistanceSample * 0.01f, Remaining * 0.01f));
			Check(Site->IsWithinHoverVolume(Craft),
				FString::Printf(TEXT("reaches the deployment zone (%.0fm from hover point)"),
					Remaining * 0.01f));
		}
		Advance(EPhase::ArrivedAtSite, 1.5f);
		break;
	}

	case EPhase::ArrivedAtSite:
	{
		AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;
		if (Site && Craft)
		{
			Check(IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("settles into a deployable hover over the site"));
		}
		// Changing target is fine right up until the player commits to the drop.
		Check(Mission && Mission->SelectSite(Site),
			TEXT("can still retarget while inbound"));

		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M3: deployment --"));

		// The assist should have pulled the craft onto the hover point by now.
		if (Site && Craft)
		{
			const float Offset = FVector::Dist(Craft->GetActorLocation(), Site->GetHoverPoint());
			Check(Offset < 400.f,
				FString::Printf(TEXT("assist parks the craft on the hover point (%.0fcm)"), Offset));
		}
		Check(Controller->CanDeploy(), TEXT("deployment is available in the zone"));
		Check(Controller->GetDeploymentBlocker().IsEmpty(), TEXT("no blocker reported in the zone"));

		Advance(EPhase::DeploymentAssist, 0.f);
		break;
	}

	case EPhase::DeploymentAssist:
	{
		AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;

		DistanceSample = Craft ? Craft->GetActorLocation().Z : 0.f;

		Check(FMath::IsNearlyZero(Controller->GetScreenFlashAlpha()),
			TEXT("no screen flash before deploying"));

		Check(Controller->RequestDeployment(), TEXT("can deploy from the hover volume"));

		// The wash has to be opaque on the frame the swap happens, or it is not
		// hiding anything.
		Check(Controller->GetScreenFlashAlpha() >= 0.99f,
			FString::Printf(TEXT("deploying flashes the screen (alpha %.2f)"),
				Controller->GetScreenFlashAlpha()));
		Check(Mission && !Mission->SelectSite(Site), TEXT("cannot retarget once deployed"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Deploying,
			TEXT("mission is Deploying during the drop"));
		Check(Controller->GetControlMode() == EEOControlMode::Operative,
			TEXT("player is possessing the operative during the drop"));

		if (AEOOperativeCharacter* Op = Cast<AEOOperativeCharacter>(Controller->GetPawn()))
		{
			Check(Op->IsDeploying(), TEXT("operative reports deploying"));
			Check(!Op->HasControl(), TEXT("ground input is locked during the descent"));
			Check(!Op->IsStowed(), TEXT("operative is unstowed for the drop"));
		}

		Advance(EPhase::DeploymentDrop, 4.0f);
		break;
	}

	case EPhase::DeploymentDrop:
	{
		AEOOperativeCharacter* Op = Cast<AEOOperativeCharacter>(Controller->GetPawn());
		Check(Op != nullptr, TEXT("still possessing the operative after the drop"));

		if (Op)
		{
			Check(!Op->IsDeploying(), TEXT("drop finishes"));
			Check(Op->HasControl(), TEXT("ground controls become active on landing"));
			Check(Op->GetActorLocation().Z < DistanceSample,
				FString::Printf(TEXT("operative ends up below the aircraft (%.0f -> %.0f)"),
					DistanceSample, Op->GetActorLocation().Z));
		}

		Check(Mission && Mission->GetMissionState() == EEOMissionState::OnGround,
			TEXT("landing leaves the mission OnGround"));

		Check(FMath::IsNearlyZero(Controller->GetScreenFlashAlpha()),
			FString::Printf(TEXT("the flash fades out (alpha %.2f)"),
				Controller->GetScreenFlashAlpha()));

		Advance(EPhase::DeploymentLanded, 0.f);
		break;
	}

	case EPhase::DeploymentLanded:
	{
		// The aircraft is parked where it was left, not flying itself.
		if (BootPawn)
		{
			Check(FMath::IsNearlyZero(
				IEOAircraftControlInterface::Execute_GetCurrentSpeed(BootPawn), 5.f),
				TEXT("aircraft is held still while the operative is deployed"));
		}

		Advance(EPhase::Extract, 0.f);
		break;
	}

	case EPhase::Extract:
	{
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- extraction handover --"));

		Check(Controller->RequestExtraction(), TEXT("extraction accepted"));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("extraction returns control to the aircraft"));

		if (AEOAircraftPawn* Plane = Cast<AEOAircraftPawn>(GetAircraftPawn()))
		{
			// The deployment freeze must be released, or the player is handed back
			// a craft that silently ignores every input for the rest of the game.
			IEOAircraftControlInterface::Execute_SetFlightInput(Plane, FVector(1.f, 0.f, 0.f));
			SpeedSample = 0.f;
		}
		Advance(EPhase::Done, 1.5f);
		break;
	}

	// ---- M4: ground movement (run with -EOGroundTest in L_MissionTest) --------
	case EPhase::GroundSetup:
	{
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M4: ground movement --"));

		Check(Controller->PossessOperative(), TEXT("can possess the operative"));

		AEOOperativeCharacter* Op = GetOperative();
		Check(Op != nullptr, TEXT("operative is possessed"));

		if (Op)
		{
			IEODeployableInterface::Execute_SetStowed(Op, false);
			Check(Op->HasControl(), TEXT("operative has ground control"));
			Check(Op->GetTraversal() != nullptr, TEXT("operative has a traversal component"));
		}

		// Stand a stride in front of the 90cm wall, facing it.
		CheckTraversalAt(FVector(1625.f, 0.f, 96.f), 0.f, EEOTraversalType::Vault, TEXT("low wall"));
		Advance(EPhase::VaultCheck, 1.0f);
		break;
	}

	case EPhase::VaultCheck:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (Op && Op->GetTraversal())
		{
			Check(!Op->GetTraversal()->IsTraversing(), TEXT("vault completes"));
			Check(Op->GetActorLocation().X > 1800.f,
				FString::Printf(TEXT("vault ends past the wall (x=%.0f)"), Op->GetActorLocation().X));

			// Collision left disabled would be catastrophic and invisible.
			Check(Op->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision,
				TEXT("collision is restored after a vault"));
		}

		CheckTraversalAt(FVector(4350.f, 0.f, 96.f), 0.f, EEOTraversalType::Mantle, TEXT("180cm ledge"));
		Advance(EPhase::MantleCheck, 1.4f);
		break;
	}

	case EPhase::MantleCheck:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (Op && Op->GetTraversal())
		{
			Check(!Op->GetTraversal()->IsTraversing(), TEXT("mantle completes"));
			Check(Op->GetActorLocation().Z > 200.f,
				FString::Printf(TEXT("mantle ends on top of the ledge (z=%.0f)"),
					Op->GetActorLocation().Z));
			Check(Op->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision,
				TEXT("collision is restored after a mantle"));
		}

		CheckTraversalAt(FVector(5950.f, -300.f, 96.f), 0.f, EEOTraversalType::Climb, TEXT("380cm wall"));
		Advance(EPhase::ClimbCheck, 1.8f);
		break;
	}

	case EPhase::ClimbCheck:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (Op && Op->GetTraversal())
		{
			Check(!Op->GetTraversal()->IsTraversing(), TEXT("climb completes"));
			Check(Op->GetActorLocation().Z > 400.f,
				FString::Printf(TEXT("climb ends on top of the wall (z=%.0f)"),
					Op->GetActorLocation().Z));
			Check(Op->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision,
				TEXT("collision is restored after a climb"));
		}

		// Open floor with nothing to traverse must not report a false positive.
		if (Op && Op->GetTraversal())
		{
			Op->SetActorLocationAndRotation(FVector(600.f, 0.f, 96.f), FRotator::ZeroRotator,
				false, nullptr, ETeleportType::TeleportPhysics);
			Check(!Op->GetTraversal()->Scan().IsValid(),
				TEXT("open ground reports nothing to traverse"));
		}

		Advance(EPhase::StowInterrupt, 0.5f);
		break;
	}

	case EPhase::StowInterrupt:
	{
		AEOOperativeCharacter* Op = GetOperative();
		// Stowing mid-traversal must not leave collision on: the order of cancel
		// and disable decides whether a hidden operative blocks the world.
		if (Op && Op->GetTraversal())
		{
			CheckTraversalAt(FVector(1625.f, 0.f, 96.f), 0.f, EEOTraversalType::Vault,
				TEXT("stow-interrupt vault"));

			IEODeployableInterface::Execute_SetStowed(Op, true);
			Check(!Op->GetTraversal()->IsTraversing(), TEXT("stowing cancels a traversal"));
			Check(!Op->GetActorEnableCollision(),
				TEXT("a stowed operative has no collision, even mid-traversal"));

			IEODeployableInterface::Execute_SetStowed(Op, false);
			Check(Op->GetActorEnableCollision(), TEXT("unstowing restores collision"));
		}

		Advance(EPhase::SlideCheck, 0.5f);
		break;
	}

	case EPhase::SlideCheck:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (Op)
		{
			Op->SetActorLocationAndRotation(FVector(600.f, 0.f, 96.f), FRotator::ZeroRotator,
				false, nullptr, ETeleportType::TeleportPhysics);

			if (UCharacterMovementComponent* Movement = Op->GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Walking);

				// Standing still: a crouch press is a crouch, not a slide.
				Movement->Velocity = FVector::ZeroVector;
				Check(!Op->TryStartSlide(), TEXT("cannot slide from a standstill"));

				// At sprint speed it commits to a slide.
				Movement->Velocity = FVector(900.f, 0.f, 0.f);
				Check(Op->TryStartSlide(), TEXT("sliding starts at sprint speed"));
				Check(Op->IsSliding(), TEXT("operative reports sliding"));

				Check(Movement->CanEverCrouch(), TEXT("the operative is allowed to crouch"));
				Check(Movement->MaxWalkSpeedCrouched > 500.f,
					FString::Printf(TEXT("crouched speed cap allows the slide (%.0f)"),
						Movement->MaxWalkSpeedCrouched));
			}
		}
		// Crouch state resolves on the movement component's next tick, not on the
		// Crouch() call, so it is confirmed a phase later.
		Advance(EPhase::SlideConfirm, 0.4f);
		break;
	}

	case EPhase::SlideConfirm:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (Op)
		{
			Check(Op->bIsCrouched, TEXT("sliding actually crouches the capsule"));

			Op->StopSlide();
			Check(!Op->IsSliding(), TEXT("slide can be ended"));

			if (UCharacterMovementComponent* Movement = Op->GetCharacterMovement())
			{
				Check(FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 500.f, 1.f),
					FString::Printf(TEXT("walk speed restored after a slide (%.0f)"),
						Movement->MaxWalkSpeed));
			}
		}

		Advance(EPhase::MeshDriftRun, 0.2f);
		break;
	}

	case EPhase::MeshDriftRun:
	{
		// The locomotion clips carry real root motion - the jog loop travels 22
		// metres - and single-node playback evaluates the root bone without
		// consuming it. Unlocked, that walks the rendered model clean off the
		// capsule the game is actually moving, and no other check here would
		// notice, because every one of them reads the capsule.
		//
		// Each clip is posed directly rather than run through the character:
		// waiting for the operative to accelerate into a jog made the check
		// measure an idle standing still, which passed and proved nothing.
		AEOOperativeCharacter* Op = GetOperative();
		USkeletalMeshComponent* MeshComp = Op ? Op->GetMesh() : nullptr;

		Check(MeshComp != nullptr, TEXT("operative has a mesh to check"));
		if (MeshComp)
		{
			// PlayAnimation switches the component to single-node playback, and
			// nothing switches it back. Left alone, this check quietly disables the
			// Animation Blueprint for every phase that follows it - and for anyone
			// watching the run, which is how it was found.
			const EAnimationMode::Type PreviousMode = MeshComp->GetAnimationMode();

			const TArray<UAnimSequence*> Clips = Op->GetLocomotionClips();
			Check(Clips.Num() >= 3, FString::Printf(
				TEXT("the locomotion clips are assigned (%d)"), Clips.Num()));

			for (UAnimSequence* Clip : Clips)
			{
				MeshComp->PlayAnimation(Clip, false);

				// Most of the way through, where a clip that travels has travelled.
				MeshComp->SetPosition(Clip->GetPlayLength() * 0.9f, false);
				MeshComp->RefreshBoneTransforms();

				const FVector RootWorld =
					MeshComp->GetBoneLocation(TEXT("root"), EBoneSpaces::WorldSpace);
				const float Drift = FVector::Dist(RootWorld, MeshComp->GetComponentLocation());

				Check(Drift < 25.f, FString::Printf(
					TEXT("%s keeps its root on the capsule (%.0fcm)"), *Clip->GetName(), Drift));
			}

			// Hand the mesh back as it was found. Re-applying the mode is what
			// rebuilds the Animation Blueprint's instance.
			if (PreviousMode != EAnimationMode::AnimationSingleNode)
			{
				MeshComp->SetAnimationMode(PreviousMode);
			}
		}

		Advance(EPhase::MeshDriftCheck, 0.2f);
		break;
	}

	case EPhase::MeshDriftCheck:
	{
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M5: one guard --"));

		// The traversal checks teleport the operative across the whole route, so
		// the guard has almost certainly seen something by now. Start clean.
		if (AEOOperativeCharacter* Clear = GetOperative())
		{
			Clear->SetActorLocation(FVector(0.f, 6000.f, 96.f), false, nullptr,
				ETeleportType::TeleportPhysics);
		}
		ResetEncounter();
		Advance(EPhase::GuardReset, 1.0f);
		break;
	}

	case EPhase::GuardReset:
	{
		ResetEncounter();
		Advance(EPhase::GuardPatrol, 0.5f);
		break;
	}

	case EPhase::GuardPatrol:
	{
		AEOGuardCharacter* Guard = GetGuard();
		Check(Guard != nullptr, TEXT("the level has a guard"));

		if (!Guard)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		Check(Guard->GetHealth() != nullptr, TEXT("guard has health"));
		Check(Guard->GetGuardState() == EEOGuardState::Patrolling, TEXT("guard starts on patrol"));
		Check(!Guard->IsDead(), TEXT("guard starts alive"));
		Check(!Guard->CanSeeTarget(), TEXT("guard sees nothing with the player far away"));

		DistanceSample = Guard->GetActorLocation().X;
		Advance(EPhase::GuardStealthKill, 2.5f);
		break;
	}

	// ---- Outcome 1: successful stealth kill ----------------------------------
	case EPhase::GuardStealthKill:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		Check(!FMath::IsNearlyEqual(Guard->GetActorLocation().X, DistanceSample, 20.f),
			TEXT("guard walks its patrol"));
		Check(Guard->GetDetectionAlpha() <= 0.f, TEXT("guard has not noticed anything"));

		const FVector GuardLocation = Guard->GetActorLocation();
		const FVector Behind = GuardLocation - Guard->GetActorForwardVector() * 130.f;
		const FVector InFront = GuardLocation + Guard->GetActorForwardVector() * 130.f;

		Op->SetActorLocation(InFront, false, nullptr, ETeleportType::TeleportPhysics);
		Check(!Guard->CanBeTakenDownBy(Op), TEXT("cannot take down a guard from the front"));

		Op->SetActorLocation(GuardLocation + Guard->GetActorRightVector() * 900.f,
			false, nullptr, ETeleportType::TeleportPhysics);
		Check(!Guard->CanBeTakenDownBy(Op), TEXT("cannot take down a guard from out of reach"));

		Op->SetActorLocation(Behind, false, nullptr, ETeleportType::TeleportPhysics);
		Check(Guard->CanBeTakenDownBy(Op), TEXT("can take down an unaware guard from behind"));
		Check(Op->FindTakedownTarget() == Guard, TEXT("takedown finds the guard"));

		Check(Op->TryTakedown(), TEXT("takedown succeeds"));
		Check(Guard->IsDead(), TEXT("stealth kill kills the guard"));
		Check(!Op->TryTakedown(), TEXT("cannot take down a corpse"));

		// Stand clear before reviving: left where it is, the operative is right
		// behind the guard and gets spotted the instant the guard comes back.
		Op->SetActorLocation(FVector(0.f, 6000.f, 96.f), false, nullptr,
			ETeleportType::TeleportPhysics);
		ResetEncounter();
		Advance(EPhase::GuardSeesPlayer, 0.6f);
		break;
	}

	// ---- Outcome 2: botched stealth - the guard sees the player ---------------
	case EPhase::GuardSeesPlayer:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		Check(!Guard->IsDead(), TEXT("guard is alive again after a reset"));
		Check(Guard->GetGuardState() == EEOGuardState::Patrolling,
			TEXT("reset returns the guard to patrol"));

		Op->SetActorLocation(Guard->GetActorLocation() + Guard->GetActorForwardVector() * 600.f,
			false, nullptr, ETeleportType::TeleportPhysics);

		Advance(EPhase::GuardAlerted, 1.6f);
		break;
	}

	case EPhase::GuardAlerted:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		Check(Guard->CanSeeTarget(), TEXT("guard sees a player standing in front of it"));
		Check(Guard->GetDetectionAlpha() >= 1.f,
			FString::Printf(TEXT("detection fills (%.2f)"), Guard->GetDetectionAlpha()));
		Check(Guard->GetGuardState() == EEOGuardState::Alerted, TEXT("guard becomes alerted"));

		// Being seen is what closes off the silent option.
		Op->SetActorLocation(Guard->GetActorLocation() - Guard->GetActorForwardVector() * 130.f,
			false, nullptr, ETeleportType::TeleportPhysics);
		Check(!Guard->CanBeTakenDownBy(Op), TEXT("an alerted guard cannot be taken down"));

		Op->SetActorLocation(Guard->GetActorLocation() + Guard->GetActorForwardVector() * 700.f,
			false, nullptr, ETeleportType::TeleportPhysics);
		SpeedSample = Op->GetHealth() ? Op->GetHealth()->GetHealth() : 0.f;

		Advance(EPhase::GuardShootsPlayer, 3.0f);
		break;
	}

	// ---- Outcome 3: short firefight, and the player can die -------------------
	case EPhase::GuardShootsPlayer:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op || !Op->GetHealth())
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		Check(Op->GetHealth()->GetHealth() < SpeedSample,
			FString::Printf(TEXT("guard shoots the player (%.0f -> %.0f health, guard %s, sees=%d, range=%.0f)"),
				SpeedSample, Op->GetHealth()->GetHealth(),
				*StaticEnum<EEOGuardState>()->GetNameStringByValue(
					static_cast<int64>(Guard->GetGuardState())),
				Guard->CanSeeTarget() ? 1 : 0,
				FVector::Dist(Guard->GetActorLocation(), Op->GetActorLocation())));

		// A mission that never started cannot fail, and the ground suite runs
		// without one, so start it before testing the death consequence.
		if (Mission && Mission->GetMissionState() == EEOMissionState::Inactive)
		{
			Mission->SelectDefaultSite();
			Mission->StartMission();
			Mission->BeginDeployment();
			Mission->CompleteDeployment();
		}

		Op->GetHealth()->ApplyDamage(Op->GetHealth()->GetMaxHealth() * 2.f, Guard);
		Check(Op->IsDead(), TEXT("the player can be killed"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Failed,
			TEXT("the player dying fails the mission"));

		ResetEncounter();
		if (Mission)
		{
			Mission->ResetMission();
		}
		Advance(EPhase::GuardGunKill, 0.5f);
		break;
	}

	// ---- Outcome 4: gun kill --------------------------------------------------
	case EPhase::GuardGunKill:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op || !Op->GetHealth() || !Guard->GetHealth())
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		Check(!Op->IsDead(), TEXT("the operative revives for the next attempt"));
		Check(!Guard->IsDead(), TEXT("the guard revives for the next attempt"));

		// Pin the guard first. Shooting at a patrolling target puts the camera in a
		// different place every run, and the aim trace can slip past it.
		Guard->ResetGuard();

		// Close range on purpose. Hip spread is 4.5 degrees - at 9m that cone is
		// +/-71cm against a 42cm capsule, so a single shot misses more often than
		// it lands however well it is aimed. At 3m the cone is narrower than the
		// target, so the check measures the damage path instead of the dice.
		Op->SetActorLocation(Guard->GetActorLocation() + Guard->GetActorForwardVector() * 300.f,
			false, nullptr, ETeleportType::TeleportPhysics);

		// The weapon aims where the CAMERA looks, and the camera boom follows the
		// control rotation - so pointing the body at the guard is not enough, and
		// the boom only picks the new rotation up on the pawn's next tick.
		const FRotator AimAt = (Guard->GetActorLocation() - Op->GetActorLocation()).Rotation();
		Op->SetActorRotation(FRotator(0.f, AimAt.Yaw, 0.f));
		Controller->SetControlRotation(AimAt);

		Advance(EPhase::GuardGunAim, 0.2f);
		break;
	}

	case EPhase::GuardGunAim:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		// Put the crosshair on the guard, iteratively.
		//
		// The chase boom hangs off the operative's shoulder, so rotating to aim
		// also MOVES the camera - one correction does not converge. This is the
		// same loop a player closes by hand, and getting it on target is itself
		// worth asserting: if the crosshair cannot be put on a guard standing in
		// the open, the weapon is unusable regardless of what the traces do.
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

		// How close the aim ray passes to the guard's centre line, not merely
		// whether it clips the capsule. Stopping at the first grazing hit centres
		// the weapon's spread cone on the rim, and half the cone then falls
		// outside the target.
		const FVector ToGuard = Guard->GetActorLocation() - ViewLocation;
		const FVector AimDirection = ViewRotation.Vector();
		const float MissDistance =
			FVector::CrossProduct(ToGuard, AimDirection).Size() / FMath::Max(AimDirection.Size(), KINDA_SMALL_NUMBER);

		const bool bOnTarget = MissDistance < 15.f;

		if (bOnTarget || PhaseElapsed > 3.f)
		{
			Check(bOnTarget,
				FString::Printf(TEXT("the crosshair settles on the guard (%.0fcm off centre)"),
					MissDistance));

			// Fire on this exact step. The follow camera has positional lag and
			// is still easing after the teleport, so waiting even a tenth of a
			// second lets the crosshair slide off the guard again.
			if (Guard->GetHealth())
			{
				const float GuardStart = Guard->GetHealth()->GetHealth();

				// Take the dice out of the shot. Hip spread is 4.5 degrees and the
				// cone is centred on the aim line, so even a crosshair dead on the
				// guard misses some of the time - which is correct for the weapon
				// and useless in a check that is asking whether damage reaches the
				// guard at all. That was the cause of this check's long history of
				// failing intermittently, and it is only fixable now that the
				// weapon has an interface to ask.
				//
				// The cone itself is asserted separately and deterministically, in
				// ExecutiveOps.Weapon.Spread stays inside its cone.
				UEOWeaponComponent* Weapon = Op->GetWeapon();
				const float SavedHipSpread = Weapon ? Weapon->HipSpread : 0.f;
				const float SavedAimSpread = Weapon ? Weapon->AimSpread : 0.f;
				if (Weapon)
				{
					Weapon->HipSpread = 0.f;
					Weapon->AimSpread = 0.f;
				}

				Check(Op->FireWeapon(), TEXT("the weapon fires"));
				Check(Guard->GetHealth()->GetHealth() < GuardStart,
					FString::Printf(TEXT("a real shot damages the guard (%.0f -> %.0f)"),
						GuardStart, Guard->GetHealth()->GetHealth()));
				Check(!Guard->IsDead(), TEXT("one pistol hit does not kill"));

				if (Weapon)
				{
					Weapon->HipSpread = SavedHipSpread;
					Weapon->AimSpread = SavedAimSpread;
				}

				Guard->GetHealth()->ApplyDamage(55.f, Op);
				Check(Guard->IsDead(), TEXT("two pistol hits kill the guard"));
				Check(Guard->GetHealth()->ApplyDamage(55.f, Op) == 0.f,
					TEXT("a corpse takes no further damage"));
			}

			ResetEncounter();
			Advance(EPhase::GuardLosesPlayer, 0.5f);
			break;
		}

		// Hold the guard still while the aim converges; a patrolling target moves
		// the solution every iteration.
		Guard->ResetGuard();
		Controller->SetControlRotation((Guard->GetActorLocation() - ViewLocation).Rotation());
		return;
	}

	case EPhase::GuardGunFire:
		// Folded into GuardGunAim, which fires the moment the crosshair is on
		// target rather than a phase later.
		Advance(EPhase::GuardLosesPlayer, 0.f);
		break;

	case EPhase::GuardLosesPlayer:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		SpeedSample = Guard->GetDetectionAlpha();
		Op->SetActorLocation(FVector(0.f, 8000.f, 96.f), false, nullptr, ETeleportType::TeleportPhysics);

		// Perception is evaluated on the guard's tick, so the result of a teleport
		// cannot be read in the same frame.
		Advance(EPhase::GuardLostConfirm, 1.0f);
		break;
	}

	case EPhase::GuardLostConfirm:
	{
		AEOGuardCharacter* Guard = GetGuard();
		if (Guard)
		{
			Check(!Guard->CanSeeTarget(), TEXT("guard cannot see a player who has broken contact"));
			Check(Guard->GetDetectionAlpha() < 1.f, TEXT("awareness decays once contact is broken"));
		}
		UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] -- M6: the mission, run twice --"));
		MissionAttempt = 1;
		Advance(EPhase::MissionSetup, 0.f);
		break;
	}

	case EPhase::MissionSetup:
	{
		// The ground suite drives the mission directly, so make sure the player
		// is actually holding the operative before each run.
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

		const FString Pass = FString::Printf(TEXT("run %d:"), MissionAttempt);

		Check(Objective != nullptr, FString::Printf(TEXT("%s the arena has an objective"), *Pass));
		Check(Zone != nullptr, FString::Printf(TEXT("%s the arena has an extraction zone"), *Pass));

		if (!Op || !Objective || !Zone)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		ResetEncounter();
		Check(EnterGroundMission(), FString::Printf(TEXT("%s insertion reaches OnGround"), *Pass));

		// The objective must be fresh, or the second run has nothing to do.
		Check(!Objective->IsComplete(), FString::Printf(TEXT("%s objective starts incomplete"), *Pass));

		// Extraction must be dark until the objective is done, or the player is
		// being told to leave before they have done anything.
		Check(!IEOExtractionInterface::Execute_IsExtractionAvailable(Zone),
			FString::Printf(TEXT("%s extraction is unavailable before the objective"), *Pass));

		// Standing at the extraction point early does nothing.
		Op->SetActorLocation(Zone->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		Check(Op->FindInteractable() == nullptr,
			FString::Printf(TEXT("%s nothing to interact with at a dormant extraction"), *Pass));

		Advance(EPhase::MissionObjective, 0.2f);
		break;
	}

	case EPhase::MissionObjective:
	{
		AEOOperativeCharacter* Op = GetOperative();
		AEOObjectiveTerminal* Objective = GetObjective();
		AEOExtractionZone* Zone = GetExtractionZone();

		const FString Pass = FString::Printf(TEXT("run %d:"), MissionAttempt);

		if (!Op || !Objective || !Zone)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		// Cross the arena to the terminal.
		Op->SetActorLocation(Objective->GetActorLocation() + FVector(200.f, 0.f, 0.f),
			false, nullptr, ETeleportType::TeleportPhysics);

		Check(Op->FindInteractable() == Objective,
			FString::Printf(TEXT("%s the terminal offers itself when reached"), *Pass));
		Check(Op->TryInteract(), FString::Printf(TEXT("%s interacting completes the objective"), *Pass));
		Check(Objective->IsComplete(), FString::Printf(TEXT("%s objective reports complete"), *Pass));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::ObjectiveComplete,
			FString::Printf(TEXT("%s mission reaches ObjectiveComplete"), *Pass));

		// Doing it twice must not double-fire the transition.
		Check(!Op->TryInteract(), FString::Printf(TEXT("%s the objective cannot be done twice"), *Pass));

		Check(IEOExtractionInterface::Execute_IsExtractionAvailable(Zone),
			FString::Printf(TEXT("%s extraction arms once the objective is done"), *Pass));

		Advance(EPhase::MissionExtract, 0.2f);
		break;
	}

	case EPhase::MissionExtract:
	{
		AEOOperativeCharacter* Op = GetOperative();
		AEOExtractionZone* Zone = GetExtractionZone();

		const FString Pass = FString::Printf(TEXT("run %d:"), MissionAttempt);

		if (!Op || !Zone)
		{
			Advance(EPhase::Done, 0.f);
			break;
		}

		// Run back across the arena to the pad.
		Op->SetActorLocation(Zone->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		Check(Zone->IsWithinZone(Op), FString::Printf(TEXT("%s standing in the extraction zone"), *Pass));
		Check(Op->FindInteractable() == Zone,
			FString::Printf(TEXT("%s the pad offers extraction"), *Pass));

		// Park the aircraft a long way off, so the arrival is a real flight in
		// rather than a craft that happens to already be overhead.
		if (APawn* Inbound = FindAircraftInLevel())
		{
			Inbound->SetActorLocation(Op->GetActorLocation() + FVector(-9000.f, -6000.f, 5000.f),
				false, nullptr, ETeleportType::TeleportPhysics);
			IEOAircraftControlInterface::Execute_ResetFlightState(Inbound);
		}

		Check(Op->TryInteract(), FString::Printf(TEXT("%s calling extraction works"), *Pass));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Extracting,
			FString::Printf(TEXT("%s mission enters Extracting"), *Pass));
		Check(Controller->IsExtractionInbound(),
			FString::Printf(TEXT("%s the aircraft is inbound"), *Pass));
		Check(Controller->GetControlMode() == EEOControlMode::Operative,
			FString::Printf(TEXT("%s the operative stays playable while it comes"), *Pass));

		DistanceSample = Controller->GetExtractionDistance();
		Check(DistanceSample > 50.f,
			FString::Printf(TEXT("%s the aircraft starts a long way out (%.0fm)"),
				*Pass, DistanceSample));

		// Polled, not timed: the mission re-arms itself a few seconds after it
		// completes, so a fixed dwell would sample the re-armed state instead.
		Advance(EPhase::MissionPickup, 0.f);
		break;
	}

	case EPhase::MissionPickup:
	{
		// Wait for the arrival, up to a generous bound.
		if (Controller->IsExtractionInbound() && PhaseElapsed < 30.f)
		{
			return;
		}

		const FString Pass = FString::Printf(TEXT("run %d:"), MissionAttempt);

		Check(!Controller->IsExtractionInbound(),
			FString::Printf(TEXT("%s the extraction finishes"), *Pass));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			FString::Printf(TEXT("%s the pickup returns control to the aircraft"), *Pass));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Complete,
			FString::Printf(TEXT("%s mission reaches Complete"), *Pass));

		// And the loop closes: the craft the player is handed back must fly.
		if (APawn* Departing = GetAircraftPawn())
		{
			IEOAircraftControlInterface::Execute_SetFlightInput(Departing, FVector(1.f, 0.f, 0.f));
		}

		if (MissionAttempt >= 2)
		{
			Advance(EPhase::Done, 1.5f);
			break;
		}

		// The whole point of M6: it has to be playable again without debug help.
		UE_LOG(LogExecutiveOpsDev, Display,
			TEXT("[SelfTest] -- M6: resetting and running the mission again --"));
		Controller->EOReset();
		MissionAttempt = 2;
		Advance(EPhase::MissionSecondRun, 0.5f);
		break;
	}

	case EPhase::MissionSecondRun:
	{
		// EOReset put the player back in the aircraft; the ground suite needs the
		// operative again before the second run can start.
		Check(Controller->PossessOperative(), TEXT("run 2: can possess the operative again"));

		if (AEOOperativeCharacter* Op = GetOperative())
		{
			IEODeployableInterface::Execute_SetStowed(Op, false);
		}
		Advance(EPhase::MissionSetup, 0.2f);
		break;
	}

	case EPhase::Done:
	default:
		if (APawn* Plane = GetAircraftPawn())
		{
			const float Speed = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane);
			Check(Speed > 100.f,
				FString::Printf(TEXT("aircraft flies away after extraction (%.0f cm/s)"), Speed));
		}
		Finish();
		break;
	}
}

void UEOSelfTest::SteerTowardSite()
{
	AEOMissionSite* Site = Controller
		? Controller->GetWorld()->GetSubsystem<UEOMissionSubsystem>()->GetSelectedSite()
		: nullptr;
	APawn* Craft = GetAircraftPawn();
	if (!Site || !Craft)
	{
		return;
	}

	// Cruise above the tallest tower in the district, then descend on finals.
	// A straight line from the start to the site passes through the tower grid,
	// so a naive direct approach just scrapes down the side of a building.
	constexpr float CruiseAltitude = 12000.f;
	constexpr float OverheadRange = 3000.f;
	constexpr float SlowRange = 6000.f;
	constexpr float StopRange = 400.f;

	const FVector Location = Craft->GetActorLocation();
	const FVector Target = Site->GetHoverPoint();

	const FVector Offset = Target - Location;
	const float HorizontalDistance = FVector(Offset.X, Offset.Y, 0.f).Size();

	// Point the nose at the site. The craft yaws itself in play; here the test
	// stands in for the pilot.
	if (HorizontalDistance > StopRange)
	{
		Craft->SetActorRotation(FRotator(0.f, Offset.Rotation().Yaw, 0.f));
	}

	// Stay above the towers until almost overhead, then descend more or less
	// vertically. Descending on the way in flies the craft into the side of the
	// neighbouring block.
	const bool bOverhead = HorizontalDistance < OverheadRange;
	const float DesiredZ = bOverhead ? Target.Z : FMath::Max(Target.Z, CruiseAltitude);

	IEOAircraftControlInterface::Execute_SetHoverEnabled(Craft, HorizontalDistance < SlowRange);

	if (Offset.Size() < StopRange)
	{
		IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector::ZeroVector);
		return;
	}

	const float AltitudeError = DesiredZ - Location.Z;
	const float Climb = FMath::Clamp(AltitudeError / 800.f, -1.f, 1.f);

	// Do not cross the district until the climb is done, and ease off forward
	// once overhead so the descent is not also a fly-past.
	float Forward = 1.f;
	if (!bOverhead && AltitudeError > 2000.f)
	{
		Forward = 0.f;
	}
	else if (bOverhead)
	{
		Forward = FMath::Clamp(HorizontalDistance / OverheadRange, 0.f, 1.f);
	}

	IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector(Forward, 0.f, Climb));
}

bool UEOSelfTest::CheckTraversalAt(const FVector& StandLocation, float FacingYaw,
	EEOTraversalType Expected, const TCHAR* Label)
{
	AEOOperativeCharacter* Op = GetOperative();
	UEOTraversalComponent* Traverse = Op ? Op->GetTraversal() : nullptr;
	if (!Op || !Traverse)
	{
		Check(false, FString::Printf(TEXT("%s: no operative"), Label));
		return false;
	}

	Op->SetActorLocationAndRotation(StandLocation, FRotator(0.f, FacingYaw, 0.f),
		/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	const FEOTraversalQuery Query = Traverse->Scan();

	const FString Found = StaticEnum<EEOTraversalType>()
		->GetNameStringByValue(static_cast<int64>(Query.Type));

	Check(Query.Type == Expected,
		FString::Printf(TEXT("%s is detected as %s (found %s, %.0fcm)"),
			Label,
			*StaticEnum<EEOTraversalType>()->GetNameStringByValue(static_cast<int64>(Expected)),
			*Found, Query.ObstacleHeight));

	if (Query.Type != Expected)
	{
		return false;
	}

	// The destination has to be somewhere the operative can actually be.
	Check(Query.EndLocation.Z > StandLocation.Z - 500.f,
		FString::Printf(TEXT("%s ends somewhere sane"), Label));

	// The arc has to pass OVER the obstacle. A quadratic Bezier only travels
	// halfway to its control point, so this is the check that catches an apex
	// that leaves the capsule sweeping through whatever it is clearing.
	if (Expected == EEOTraversalType::Vault)
	{
		const float ObstacleTopZ = StandLocation.Z - 96.f + Query.ObstacleHeight;
		Check(Query.ApexLocation.Z - 96.f > ObstacleTopZ - 10.f,
			FString::Printf(TEXT("%s apex clears the obstacle top"), Label));
	}

	const bool bStarted = Traverse->TryTraverse();
	Check(bStarted, FString::Printf(TEXT("%s traversal starts"), Label));
	Check(Traverse->IsTraversing(), FString::Printf(TEXT("%s is in progress"), Label));
	return bStarted;
}

void UEOSelfTest::Finish()
{
	if (Controller)
	{
		Controller->GetWorldTimerManager().ClearTimer(StepTimer);
	}

	const bool bPassed = (Failures == 0);
	UE_LOG(LogExecutiveOpsDev, Display, TEXT("[SelfTest] === %s (%d/%d checks passed) ==="),
		bPassed ? TEXT("PASSED") : TEXT("FAILED"), Checks - Failures, Checks);

	// Rooted on Start, because nothing else holds a reference once the harness
	// lives outside the game module. The run is over, so let it go.
	if (IsRooted())
	{
		RemoveFromRoot();
	}

	if (ShouldExitAfterRun())
	{
		FPlatformMisc::RequestExitWithStatus(/*bForce=*/false, bPassed ? 0 : 1);
	}
}
