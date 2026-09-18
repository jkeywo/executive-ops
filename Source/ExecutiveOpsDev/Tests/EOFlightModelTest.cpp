#include "Tests/EOFlightModelTest.h"

#include "ExecutiveOpsDev.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Core/EOPlayerController.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EOMissionTypes.h"
#include "Mission/EOMissionSubsystem.h"

#include "Engine/World.h"

AEOFlightModelTest::AEOFlightModelTest()
{
	TimeLimit = 30.f;
}

void AEOFlightModelTest::Step()
{
	AEOPlayerController* Controller = GetController();
	if (!Controller)
	{
		Check(false, TEXT("no player controller"));
		Done();
		return;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	APawn* Craft = GetAircraftPawn();

	switch (Current())
	{
	// ---- M0: boots into a controllable aircraft ------------------------------
	case EPhase::BootState:
	{
		Check(Mission != nullptr, TEXT("mission subsystem exists"));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("starts possessing the aircraft"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Inactive,
			TEXT("starts with mission Inactive"));
		Check(Craft != nullptr, TEXT("possessed pawn implements the aircraft interface"));
		Check(Mission && Mission->SelectDefaultSite() != nullptr,
			TEXT("the level provides a mission site"));
		Go(EPhase::DeployGate, 0.f);
		return;
	}

	case EPhase::DeployGate:
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
		Go(EPhase::Reset, 0.f);
		return;
	}

	case EPhase::Reset:
	{
		Controller->EOReset();
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("reset leaves the player in the aircraft"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Inactive,
			TEXT("reset returns mission to Inactive"));
		Go(EPhase::IllegalTransitions, 0.f);
		return;
	}

	case EPhase::IllegalTransitions:
	{
		if (Mission)
		{
			// Both refusals are the point; the subsystem warns about each.
			ExpectWarning(TEXT("Mission transition to Complete rejected"));
			ExpectWarning(TEXT("Mission transition to Extracting rejected"));

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
		Go(EPhase::Accelerate, 1.5f);
		return;
	}

	// ---- M1: the craft actually flies ----------------------------------------
	case EPhase::Accelerate:
	{
		if (Craft)
		{
			SpeedSample = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Craft);
			Check(SpeedSample > 500.f,
				FString::Printf(TEXT("accelerates under sustained input (%.0f cm/s)"), SpeedSample));
			Check(!IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("cannot deploy while moving fast"));

			// Actually release the throttle, then check momentum bleeds off.
			IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector::ZeroVector);
		}
		Go(EPhase::Coast, 2.0f);
		return;
	}

	case EPhase::Coast:
	{
		if (Craft)
		{
			const float Coasted = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Craft);
			Check(Coasted < SpeedSample,
				FString::Printf(TEXT("decelerates once input is released (%.0f -> %.0f cm/s)"),
					SpeedSample, Coasted));

			// Drive back up to speed, then demand hover, to prove the clamp applies.
			IEOAircraftControlInterface::Execute_SetFlightInput(Craft, FVector(1.f, 0.f, 0.f));
		}
		Go(EPhase::HoverEngage, 1.5f);
		return;
	}

	case EPhase::HoverEngage:
	{
		if (AEOAircraftPawn* Plane = Cast<AEOAircraftPawn>(Craft))
		{
			SpeedSample = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane);
			Check(Plane->GetHoverBlend() <= KINDA_SMALL_NUMBER,
				TEXT("hover blend is zero while flying"));
			Check(Plane->GetThrustAlpha() > 0.5f, TEXT("thrust reads high under input"));

			// Hover is where the craft parks: engage it and let go of the throttle.
			IEOAircraftControlInterface::Execute_SetHoverEnabled(Plane, true);
			IEOAircraftControlInterface::Execute_SetFlightInput(Plane, FVector::ZeroVector);
		}
		Go(EPhase::HoverSpeedClamp, 1.5f);
		return;
	}

	case EPhase::HoverSpeedClamp:
	{
		if (AEOAircraftPawn* Plane = Cast<AEOAircraftPawn>(Craft))
		{
			Check(Plane->GetHoverBlend() >= 0.99f,
				FString::Printf(TEXT("hover blend reaches 1 (%.2f)"), Plane->GetHoverBlend()));

			const float Hovering = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane);
			Check(Hovering < SpeedSample,
				FString::Printf(TEXT("hover sheds speed once the throttle is released (%.0f -> %.0f cm/s)"),
					SpeedSample, Hovering));

			// Entering hover at speed must not leave the craft above the hover cap.
			Check(Hovering <= Plane->GetHoverMaxSpeed(),
				FString::Printf(TEXT("hover clamps speed to the hover maximum (%.0f cm/s)"), Hovering));

			IEOAircraftControlInterface::Execute_ResetFlightState(Plane);
			Check(FMath::IsNearlyZero(IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane)),
				TEXT("reset clears velocity"));

			// The cockpit panel follows the view. Possession already lands in first
			// person, so the switch is the part nothing else here exercises: chase
			// takes the panel off the glass and coming back puts it there again.
			Check(Plane->IsFirstPerson(), TEXT("the craft is possessed in first person"));
			Check(Plane->IsCockpitPanelShowing(), TEXT("and the cockpit panel is showing"));

			Plane->SetFirstPerson(false);
			Check(!Plane->IsCockpitPanelShowing(), TEXT("chase view takes the panel off the glass"));

			Plane->SetFirstPerson(true);
			Check(Plane->IsCockpitPanelShowing(), TEXT("first person puts it back"));
		}

		// Back to the start, for whatever runs in this map next.
		Controller->EOReset();
		Go(EPhase::Finished, 0.f);
		return;
	}

	case EPhase::Finished:
	default:
		Done();
		return;
	}
}
