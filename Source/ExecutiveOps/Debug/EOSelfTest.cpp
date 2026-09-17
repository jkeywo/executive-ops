#include "Debug/EOSelfTest.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Core/EOPlayerController.h"
#include "ExecutiveOps.h"
#include "GameFramework/Character.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Mission/EOMissionSite.h"
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

void UEOSelfTest::Check(bool bCondition, const FString& Description)
{
	++Checks;
	if (bCondition)
	{
		UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest]   PASS  %s"), *Description);
	}
	else
	{
		++Failures;
		UE_LOG(LogExecutiveOps, Error, TEXT("[SelfTest]   FAIL  %s"), *Description);
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
		UE_LOG(LogExecutiveOps, Error, TEXT("[SelfTest] no player controller"));
		return;
	}

	UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] === self-test starting ==="));

	Phase = EPhase::CoreState;
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
		UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] -- M0: boot and control --"));
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
			Check(Controller->GetDeploymentBlocker() == TEXT("outside the deployment zone"),
				FString::Printf(TEXT("deployment blocked by distance (got '%s')"),
					*Controller->GetDeploymentBlocker()));
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
		UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] -- M1: flight --"));
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
		}
		UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] -- M2: navigation --"));
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

		UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] -- M3: deployment --"));

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

		Check(Controller->RequestDeployment(), TEXT("can deploy from the hover volume"));
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
		UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] -- extraction handover --"));

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

	case EPhase::Done:
	default:
		if (APawn* Plane = GetAircraftPawn())
		{
			const float Speed = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane);
			Check(Speed > 100.f,
				FString::Printf(TEXT("aircraft flies again after extraction (%.0f cm/s)"), Speed));
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

void UEOSelfTest::Finish()
{
	if (Controller)
	{
		Controller->GetWorldTimerManager().ClearTimer(StepTimer);
	}

	const bool bPassed = (Failures == 0);
	UE_LOG(LogExecutiveOps, Display, TEXT("[SelfTest] === %s (%d/%d checks passed) ==="),
		bPassed ? TEXT("PASSED") : TEXT("FAILED"), Checks - Failures, Checks);

	if (ShouldExitAfterRun())
	{
		FPlatformMisc::RequestExitWithStatus(/*bForce=*/false, bPassed ? 0 : 1);
	}
}
