#include "Debug/EOSelfTest.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Core/EOPlayerController.h"
#include "ExecutiveOps.h"
#include "GameFramework/Character.h"
#include "Interfaces/EOAircraftControlInterface.h"
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
		Advance(EPhase::DeployGuard, 0.f);
		break;
	}

	case EPhase::DeployGuard:
	{
		if (Craft)
		{
			IEOAircraftControlInterface::Execute_SetHoverEnabled(Craft, false);
			Check(!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("not ready to deploy while not hovering"));
			Check(!Controller->RequestDeployment(),
				TEXT("deployment refused while not hovering"));

			IEOAircraftControlInterface::Execute_SetHoverEnabled(Craft, true);
			Check(IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("ready to deploy while hovering and slow"));
		}
		Advance(EPhase::Deploy, 0.f);
		break;
	}

	case EPhase::Deploy:
	{
		Check(Controller->RequestDeployment(), TEXT("deployment accepted while hovering"));
		Check(Controller->GetControlMode() == EEOControlMode::Operative,
			TEXT("deployment possesses the operative"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::OnGround,
			TEXT("deployment leaves mission OnGround"));

		APawn* Ground = Controller->GetPawn();
		Check(Ground && Ground->IsA(ACharacter::StaticClass()), TEXT("operative pawn is a Character"));
		Check(Ground != BootPawn, TEXT("operative is a different pawn to the aircraft"));
		Advance(EPhase::Extract, 0.f);
		break;
	}

	case EPhase::Extract:
	{
		Check(Controller->RequestExtraction(), TEXT("extraction accepted"));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("extraction returns control to the aircraft"));
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
		Advance(EPhase::Done, 0.f);
		break;
	}

	case EPhase::Done:
	default:
		Finish();
		break;
	}
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
