#include "Tests/EOGuardEncounterTest.h"

#include "ExecutiveOpsDev.h"

#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Combat/EOWeaponComponent.h"
#include "Core/EOPlayerController.h"
#include "Interfaces/EODeployableInterface.h"
#include "Interfaces/EOMissionTypes.h"
#include "Mission/EOMissionSubsystem.h"

#include "Engine/World.h"

namespace
{
	// Well away from the guard's patrol, so the operative is out of sight while
	// it is not the thing being tested.
	const FVector StandClear(0.f, 6000.f, 96.f);
	const FVector OutOfContact(0.f, 8000.f, 96.f);
}

AEOGuardEncounterTest::AEOGuardEncounterTest()
{
	TimeLimit = 60.f;
}

void AEOGuardEncounterTest::Step()
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
		Check(Controller->PossessOperative(), TEXT("can possess the operative"));
		AEOOperativeCharacter* Op = GetOperative();
		AEOGuardCharacter* Guard = GetGuard();
		Check(Op != nullptr, TEXT("operative is possessed"));
		Check(Guard != nullptr, TEXT("the level has a guard"));
		if (!Op || !Guard)
		{
			Done();
			return;
		}

		IEODeployableInterface::Execute_SetStowed(Op, false);
		Op->SetActorLocation(StandClear, false, nullptr, ETeleportType::TeleportPhysics);
		ResetEncounter();
		Go(EPhase::Patrol, 0.5f);
		return;
	}

	case EPhase::Patrol:
	{
		AEOGuardCharacter* Guard = GetGuard();
		if (!Guard)
		{
			Done();
			return;
		}

		Check(Guard->GetHealth() != nullptr, TEXT("guard has health"));
		Check(Guard->GetGuardState() == EEOGuardState::Patrolling, TEXT("guard starts on patrol"));
		Check(!Guard->IsDead(), TEXT("guard starts alive"));
		Check(!Guard->CanSeeTarget(), TEXT("guard sees nothing with the player far away"));

		PatrolStartX = Guard->GetActorLocation().X;
		Go(EPhase::StealthKill, 2.5f);
		return;
	}

	// ---- Outcome 1: successful stealth kill ----------------------------------
	case EPhase::StealthKill:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Done();
			return;
		}

		Check(!FMath::IsNearlyEqual(Guard->GetActorLocation().X, PatrolStartX, 20.f),
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
		Op->SetActorLocation(StandClear, false, nullptr, ETeleportType::TeleportPhysics);
		ResetEncounter();
		Go(EPhase::SeesPlayer, 0.6f);
		return;
	}

	// ---- Outcome 2: botched stealth - the guard sees the player ---------------
	case EPhase::SeesPlayer:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Done();
			return;
		}

		Check(!Guard->IsDead(), TEXT("guard is alive again after a reset"));
		Check(Guard->GetGuardState() == EEOGuardState::Patrolling,
			TEXT("reset returns the guard to patrol"));

		Op->SetActorLocation(Guard->GetActorLocation() + Guard->GetActorForwardVector() * 600.f,
			false, nullptr, ETeleportType::TeleportPhysics);

		Go(EPhase::Alerted, 1.6f);
		return;
	}

	case EPhase::Alerted:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Done();
			return;
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
		HealthSample = Op->GetHealth() ? Op->GetHealth()->GetHealth() : 0.f;

		Go(EPhase::ShootsPlayer, 3.0f);
		return;
	}

	// ---- Outcome 3: short firefight, and the player can die -------------------
	case EPhase::ShootsPlayer:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op || !Op->GetHealth())
		{
			Done();
			return;
		}

		Check(Op->GetHealth()->GetHealth() < HealthSample,
			FString::Printf(TEXT("guard shoots the player (%.0f -> %.0f health, guard %s, sees=%d, range=%.0f)"),
				HealthSample, Op->GetHealth()->GetHealth(),
				*StaticEnum<EEOGuardState>()->GetNameStringByValue(
					static_cast<int64>(Guard->GetGuardState())),
				Guard->CanSeeTarget() ? 1 : 0,
				FVector::Dist(Guard->GetActorLocation(), Op->GetActorLocation())));

		// A mission that never started cannot fail, so start one before testing
		// the death consequence.
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
		Go(EPhase::GunKill, 0.5f);
		return;
	}

	// ---- Outcome 4: gun kill --------------------------------------------------
	case EPhase::GunKill:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op || !Op->GetHealth() || !Guard->GetHealth())
		{
			Done();
			return;
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

		Go(EPhase::GunAim, 0.2f);
		return;
	}

	case EPhase::GunAim:
	{
		AEOGuardCharacter* Guard = GetGuard();
		AEOOperativeCharacter* Op = GetOperative();
		if (!Guard || !Op)
		{
			Done();
			return;
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

		if (!bOnTarget && GetPhaseElapsed() <= 3.f)
		{
			// Hold the guard still while the aim converges; a patrolling target
			// moves the solution every iteration.
			Guard->ResetGuard();
			Controller->SetControlRotation((Guard->GetActorLocation() - ViewLocation).Rotation());
			return;
		}

		Check(bOnTarget,
			FString::Printf(TEXT("the crosshair settles on the guard (%.0fcm off centre)"), MissDistance));

		// Fire on this exact step. The follow camera has positional lag and is
		// still easing after the teleport, so waiting even a tenth of a second
		// lets the crosshair slide off the guard again.
		if (Guard->GetHealth())
		{
			const float GuardStart = Guard->GetHealth()->GetHealth();

			// Take the dice out of the shot. Hip spread is 4.5 degrees and the cone
			// is centred on the aim line, so even a crosshair dead on the guard
			// misses some of the time - correct for the weapon, useless in a check
			// asking whether damage reaches the guard at all. The cone itself is
			// asserted deterministically in ExecutiveOps.Weapon.Spread stays
			// inside its cone.
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
		Go(EPhase::LosesPlayer, 0.5f);
		return;
	}

	// ---- Outcome 5: breaking contact -----------------------------------------
	case EPhase::LosesPlayer:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (!Op)
		{
			Done();
			return;
		}

		Op->SetActorLocation(OutOfContact, false, nullptr, ETeleportType::TeleportPhysics);

		// Perception is evaluated on the guard's tick, so the result of a teleport
		// cannot be read in the same frame.
		Go(EPhase::LostConfirm, 1.0f);
		return;
	}

	case EPhase::LostConfirm:
	{
		if (AEOGuardCharacter* Guard = GetGuard())
		{
			Check(!Guard->CanSeeTarget(), TEXT("guard cannot see a player who has broken contact"));
			Check(Guard->GetDetectionAlpha() < 1.f, TEXT("awareness decays once contact is broken"));
		}
		Go(EPhase::Finished, 0.f);
		return;
	}

	case EPhase::Finished:
	default:
		Done();
		return;
	}
}
