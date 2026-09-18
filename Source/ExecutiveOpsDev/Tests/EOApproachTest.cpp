#include "Tests/EOApproachTest.h"

#include "ExecutiveOpsDev.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Core/EOPlayerController.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EOMissionTypes.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOMissionSubsystem.h"

#include "Engine/World.h"

AEOApproachTest::AEOApproachTest()
{
	// The approach alone is half a minute of real flight.
	TimeLimit = 90.f;
}

void AEOApproachTest::SteerTowardSite()
{
	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;
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

void AEOApproachTest::Step()
{
	AEOPlayerController* Controller = GetController();
	if (!Controller)
	{
		Check(false, TEXT("no player controller"));
		Done();
		return;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;
	APawn* Craft = GetAircraftPawn();

	switch (Current())
	{
	// ---- M2: select a site and fly the approach -------------------------------
	case EPhase::SelectSite:
	{
		// Start from a known state whatever ran in this map before.
		Controller->EOReset();
		Craft = GetAircraftPawn();
		Site = Mission ? Mission->GetSelectedSite() : nullptr;

		Check(Craft != nullptr, TEXT("starts in the aircraft"));
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

		// Steered every step, so no dwell; the phase times itself.
		Go(EPhase::Navigate, 0.f);
		return;
	}

	case EPhase::Navigate:
	{
		SteerTowardSite();
		if (GetPhaseElapsed() < ApproachSeconds)
		{
			return;
		}

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
		Go(EPhase::Arrived, 1.5f);
		return;
	}

	case EPhase::Arrived:
	{
		// The arena was asked for when the craft entered the hover volume, and
		// the drop is gated on it being shown, so give the stream time to land.
		// The phase clock resets on every Wait, hence the separate counter.
		if (Site && !Site->IsArenaReady() && ArenaWaitSeconds < ArenaWaitLimit)
		{
			ArenaWaitSeconds += ArenaWaitStep;
			Wait(ArenaWaitStep);
			return;
		}

		if (Site && Site->HasArena())
		{
			Check(Site->IsArenaReady(),
				FString::Printf(TEXT("arena streams in at the site (%.2fs)"), ArenaWaitSeconds));
			Check(GetExtractionZone() != nullptr, TEXT("arena actors are in the world"));
		}

		// Retargeting an underway mission is answered with a refusal below, and
		// re-selecting the current site restarts nothing; both are warned about.
		ExpectWarning(TEXT("Mission transition to InFlight rejected"));
		ExpectWarning(TEXT("Site selection rejected"));

		if (Site && Craft)
		{
			Check(IEOAircraftControlInterface::Execute_IsReadyForDeployment(Craft),
				TEXT("settles into a deployable hover over the site"));
		}
		// Changing target is fine right up until the player commits to the drop.
		Check(Mission && Mission->SelectSite(Site), TEXT("can still retarget while inbound"));

		// The assist should have pulled the craft onto the hover point by now.
		if (Site && Craft)
		{
			const float Offset = FVector::Dist(Craft->GetActorLocation(), Site->GetHoverPoint());
			Check(Offset < 400.f,
				FString::Printf(TEXT("assist parks the craft on the hover point (%.0fcm)"), Offset));
		}
		Check(Controller->CanDeploy(), TEXT("deployment is available in the zone"));
		Check(Controller->GetDeploymentBlocker().IsEmpty(), TEXT("no blocker reported in the zone"));

		Go(EPhase::Deploy, 0.f);
		return;
	}

	// ---- M3: deployment -------------------------------------------------------
	case EPhase::Deploy:
	{
		AltitudeSample = Craft ? Craft->GetActorLocation().Z : 0.f;

		Check(FMath::IsNearlyZero(Controller->GetScreenFlashAlpha()),
			TEXT("no screen flash before deploying"));

		Check(Controller->RequestDeployment(), TEXT("can deploy from the hover volume"));

		// The wash has to be opaque on the frame the swap happens, or it is not
		// hiding anything.
		Check(Controller->GetScreenFlashAlpha() >= 0.99f,
			FString::Printf(TEXT("deploying flashes the screen (alpha %.2f)"), Controller->GetScreenFlashAlpha()));
		Check(Mission && !Mission->SelectSite(Site), TEXT("cannot retarget once deployed"));
		Check(Mission && Mission->GetMissionState() == EEOMissionState::Deploying,
			TEXT("mission is Deploying during the drop"));
		Check(Controller->GetControlMode() == EEOControlMode::Operative,
			TEXT("player is possessing the operative during the drop"));

		if (AEOOperativeCharacter* Op = GetOperative())
		{
			Check(Op->IsDeploying(), TEXT("operative reports deploying"));
			Check(!Op->HasControl(), TEXT("ground input is locked during the descent"));
			Check(!Op->IsStowed(), TEXT("operative is unstowed for the drop"));
		}

		Go(EPhase::Drop, 4.0f);
		return;
	}

	case EPhase::Drop:
	{
		AEOOperativeCharacter* Op = GetOperative();
		Check(Op != nullptr, TEXT("still possessing the operative after the drop"));

		if (Op)
		{
			Check(!Op->IsDeploying(), TEXT("drop finishes"));
			Check(Op->HasControl(), TEXT("ground controls become active on landing"));
			Check(Op->GetActorLocation().Z < AltitudeSample,
				FString::Printf(TEXT("operative ends up below the aircraft (%.0f -> %.0f)"),
					AltitudeSample, Op->GetActorLocation().Z));
		}

		Check(Mission && Mission->GetMissionState() == EEOMissionState::OnGround,
			TEXT("landing leaves the mission OnGround"));

		Check(FMath::IsNearlyZero(Controller->GetScreenFlashAlpha()),
			FString::Printf(TEXT("the flash fades out (alpha %.2f)"), Controller->GetScreenFlashAlpha()));

		Go(EPhase::Landed, 0.f);
		return;
	}

	case EPhase::Landed:
	{
		// The aircraft is parked where it was left, not flying itself.
		if (APawn* Parked = FindAircraftInLevel())
		{
			Check(FMath::IsNearlyZero(IEOAircraftControlInterface::Execute_GetCurrentSpeed(Parked), 5.f),
				TEXT("aircraft is held still while the operative is deployed"));
		}
		Go(EPhase::Extract, 0.f);
		return;
	}

	// ---- extraction handover ---------------------------------------------------
	case EPhase::Extract:
	{
		Check(Controller->RequestExtraction(), TEXT("extraction accepted"));
		Check(Controller->GetControlMode() == EEOControlMode::Aircraft,
			TEXT("extraction returns control to the aircraft"));

		if (APawn* Plane = GetAircraftPawn())
		{
			// The deployment freeze must be released, or the player is handed back
			// a craft that silently ignores every input for the rest of the game.
			IEOAircraftControlInterface::Execute_SetFlightInput(Plane, FVector(1.f, 0.f, 0.f));
		}
		Go(EPhase::Finished, 1.5f);
		return;
	}

	case EPhase::Finished:
	default:
	{
		if (APawn* Plane = GetAircraftPawn())
		{
			Check(IEOAircraftControlInterface::Execute_GetCurrentSpeed(Plane) > 100.f,
				TEXT("the handed-back aircraft flies"));
		}
		// Back to the start, for whatever runs in this map next.
		Controller->EOReset();
		Done();
		return;
	}
	}
}
