#include "Combat/EOGuardCharacter.h"

#include "Animation/AnimSequence.h"
#include "Character/EOOperativeCharacter.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Combat/EOGuardAIController.h"
#include "Combat/EOHealthComponent.h"
#include "Combat/EOWeaponComponent.h"
#include "Combat/EOTakedownDamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "ExecutiveOps.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"

AEOGuardCharacter::AEOGuardCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 360.f, 0.f);
	Movement->MaxWalkSpeed = PatrolSpeed;

	Health = CreateDefaultSubobject<UEOHealthComponent>(TEXT("Health"));

	// The same weapon the operative carries, tuned differently and reporting
	// itself differently. A guard's shots are weaker, slower and less accurate;
	// those were its own ShotDamage, FireInterval and ShotSpread, and they keep
	// their values here.
	Weapon = CreateDefaultSubobject<UEOWeaponComponent>(TEXT("Weapon"));
	Weapon->FireEvent = EOFeedbackEvents::Guard_Fire;
	Weapon->Damage = 18.f;
	Weapon->Interval = 0.85f;
	Weapon->HipSpread = 5.f;
	Weapon->Range = 2600.f;

	// Without a controller the movement component never runs, so a guard spawned
	// at runtime would neither move nor fall, silently.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEOGuardAIController::StaticClass();

	// Sight traces from the pawn's view point, so the eye the perception system
	// looks from is the same one the close-range check below uses.
	BaseEyeHeight = EyeHeight;
}

void AEOGuardCharacter::BeginPlay()
{
	Super::BeginPlay();

	PatrolOrigin = GetActorLocation();

	// Yaw only. A guard placed with a pitch would have a forward vector pointing
	// at the sky, which silently breaks its vision cone, its patrol heading and
	// the takedown rear-angle test.
	PatrolOriginRotation = FRotator(0.f, GetActorRotation().Yaw, 0.f);
	SetActorRotation(PatrolOriginRotation);

	if (Health)
	{
		Health->OnDied.AddDynamic(this, &AEOGuardCharacter::HandleDied);
		Health->OnDamaged.AddDynamic(this, &AEOGuardCharacter::HandleDamaged);
	}
}

// ---------------------------------------------------------------- perception

bool AEOGuardCharacter::CheckVision(AActor*& OutSeen) const
{
	OutSeen = nullptr;

	const AEOGuardAIController* AI = Cast<AEOGuardAIController>(GetController());
	if (!AI)
	{
		return false;
	}

	AEOOperativeCharacter* Operative = AI->GetOperative();
	if (!Operative || Operative->IsStowed())
	{
		// An operative that is stowed or still falling out of the aircraft is not
		// in the world as far as the guard is concerned.
		return false;
	}

	if (const UEOHealthComponent* TargetHealth = Operative->GetHealth())
	{
		if (TargetHealth->IsDead())
		{
			return false;
		}
	}

	// Range, cone and line of sight are the perception system's job now. This
	// used to be an iteration over every actor in the world followed by three
	// hand-rolled tests, once per guard per frame.
	if (AI->IsPerceiving(Operative))
	{
		OutSeen = Operative;
		return true;
	}

	// Close enough and the guard notices regardless of where they are looking:
	// walking straight into someone's back should not be free. Sight cannot
	// express this - it is the one rule that is deliberately cone-independent -
	// so it stays here, checked against the one candidate rather than against
	// the world.
	const FVector EyeLocation = GetActorLocation() + FVector(0.f, 0.f, EyeHeight);
	if (FVector::Dist(Operative->GetActorLocation(), EyeLocation) <= ProximityRange
		&& HasLineOfSightTo(*Operative))
	{
		OutSeen = Operative;
		return true;
	}

	return false;
}

bool AEOGuardCharacter::HasLineOfSightTo(const AActor& Viewed) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector EyeLocation = GetActorLocation() + FVector(0.f, 0.f, EyeHeight);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOGuardVision), false, this);
	Params.AddIgnoredActor(&Viewed);

	// Sample the torso and the head. Tracing only to the actor origin means a
	// player standing upright behind chest-high cover, head in plain view, is
	// invisible to the guard.
	static const float SampleHeights[] = { 0.f, 60.f, -40.f };

	for (const float Offset : SampleHeights)
	{
		const FVector Sample = Viewed.GetActorLocation() + FVector(0.f, 0.f, Offset);

		FHitResult Blocking;
		if (!World->LineTraceSingleByChannel(Blocking, EyeLocation, Sample, ECC_Visibility, Params))
		{
			return true;
		}
	}

	return false;
}

FVector AEOGuardCharacter::GetPursuitLocation() const
{
	// Once sight is lost the guard commits to where the target WAS. Tracking the
	// live position through a wall means breaking line of sight buys nothing,
	// which is the whole point of the stealth.
	return (bTargetVisible && Target) ? Target->GetActorLocation() : LastKnownTargetLocation;
}

void AEOGuardCharacter::HandleDamaged(float Amount, AActor* DamageInstigator,
	const UDamageType* DamageType)
{
	if (State == EEOGuardState::Dead)
	{
		return;
	}

	// A takedown is silent by definition. Reacting to it would have the guard
	// turn toward its own killer, which is the whole thing stealth is for.
	if (DamageType && DamageType->IsA<UEOTakedownDamageType>())
	{
		return;
	}

	// Being shot tells the guard both that someone is there and roughly where.
	// Without this, a non-lethal hit in the back is simply ignored.
	if (DamageInstigator)
	{
		Target = DamageInstigator;
		LastKnownTargetLocation = DamageInstigator->GetActorLocation();
	}

	DetectionAlpha = 1.f;
	TimeSinceSeen = 0.f;

	if (State != EEOGuardState::Alerted)
	{
		SetState(EEOGuardState::Alerted);
	}

	if (HitReactAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->PlayAnimation(HitReactAnim, false);
			CurrentAnim = HitReactAnim;
		}
	}
}

void AEOGuardCharacter::UpdatePerception(float DeltaSeconds)
{
	AActor* Seen = nullptr;
	bTargetVisible = CheckVision(Seen);

	if (bTargetVisible)
	{
		Target = Seen;
		LastKnownTargetLocation = Seen->GetActorLocation();
		TimeSinceSeen = 0.f;

		DetectionAlpha = FMath::Clamp(
			DetectionAlpha + DeltaSeconds / FMath::Max(TimeToDetect, KINDA_SMALL_NUMBER), 0.f, 1.f);
	}
	else
	{
		TimeSinceSeen += DeltaSeconds;

		// Awareness decays rather than snapping off, so breaking line of sight
		// briefly is a recovery rather than an instant reset.
		DetectionAlpha = FMath::Clamp(
			DetectionAlpha - DeltaSeconds / FMath::Max(TimeToLose, KINDA_SMALL_NUMBER), 0.f, 1.f);
	}
}

// -------------------------------------------------------------- state machine

void AEOGuardCharacter::SetState(EEOGuardState NewState)
{
	if (State == NewState)
	{
		return;
	}

	UE_LOG(LogExecutiveOps, Log, TEXT("Guard %s: %s -> %s"),
		*GetName(),
		*StaticEnum<EEOGuardState>()->GetNameStringByValue(static_cast<int64>(State)),
		*StaticEnum<EEOGuardState>()->GetNameStringByValue(static_cast<int64>(NewState)));

	const EEOGuardState OldState = State;
	State = NewState;

	// Escalation has to be audible from inside the player's own head: they cannot
	// see the guard's posture when running away from it. One cue per rung.
	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		FEOFeedbackContext Context = FEOFeedbackContext::AtActor(this);
		Context.AttachTo = GetRootComponent();

		switch (NewState)
		{
		case EEOGuardState::Suspicious:
			Feedback->Play(EOFeedbackEvents::Guard_Suspicious, Context);
			break;

		case EEOGuardState::Alerted:
			Feedback->Play(EOFeedbackEvents::Guard_DetectConfirmed, Context);
			break;

		case EEOGuardState::Searching:
			Feedback->Play(EOFeedbackEvents::Guard_Searching, Context);
			break;

		case EEOGuardState::Patrolling:
			// Dropping back to patrol from anywhere up the ladder is the all-clear,
			// and is the only one of these the player actively wants to hear.
			if (OldState != EEOGuardState::Dead)
			{
				Feedback->Play(EOFeedbackEvents::Guard_LostContact, Context);
			}
			break;

		case EEOGuardState::Dead:
			Feedback->Play(EOFeedbackEvents::Guard_Death, Context);
			break;

		default:
			break;
		}
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const bool bEngaged =
			(NewState == EEOGuardState::Alerted || NewState == EEOGuardState::Searching);

		Movement->MaxWalkSpeed = bEngaged ? PursuitSpeed : PatrolSpeed;

		// While engaged the guard aims where it is told. Orienting to movement
		// would overwrite that facing every tick, and both the muzzle direction
		// and the takedown rear test key off it.
		Movement->bOrientRotationToMovement =
			!(NewState == EEOGuardState::Alerted || NewState == EEOGuardState::Suspicious);
	}

	if (NewState == EEOGuardState::Alerted)
	{
		// A beat before the first shot, so being spotted is survivable if the
		// player moves immediately. Distinct from the weapon's own cooldown: this
		// is about being newly alerted, not about the rate of fire.
		FirstShotRemaining = FMath::Max(FirstShotRemaining, FirstShotDelay);
	}
	else if (NewState == EEOGuardState::Searching)
	{
		SearchRemaining = SearchDuration;
	}
}

void AEOGuardCharacter::UpdateState(float DeltaSeconds)
{
	switch (State)
	{
	case EEOGuardState::Patrolling:
		if (DetectionAlpha >= 1.f)
		{
			SetState(EEOGuardState::Alerted);
		}
		else if (DetectionAlpha > 0.f)
		{
			SetState(EEOGuardState::Suspicious);
		}
		break;

	case EEOGuardState::Suspicious:
		if (DetectionAlpha >= 1.f)
		{
			SetState(EEOGuardState::Alerted);
		}
		else if (DetectionAlpha <= 0.f)
		{
			SetState(EEOGuardState::Patrolling);
		}
		break;

	case EEOGuardState::Alerted:
		if (!bTargetVisible && TimeSinceSeen >= TimeToLose)
		{
			SetState(EEOGuardState::Searching);
		}
		break;

	case EEOGuardState::Searching:
		if (bTargetVisible && DetectionAlpha >= 1.f)
		{
			SetState(EEOGuardState::Alerted);
			break;
		}

		SearchRemaining -= DeltaSeconds;
		if (SearchRemaining <= 0.f)
		{
			// Do not zero awareness of someone standing in plain view: the HUD
			// readout would snap to empty and start climbing again from nothing.
			if (!bTargetVisible)
			{
				DetectionAlpha = 0.f;
				Target = nullptr;
			}
			SetState(EEOGuardState::Patrolling);
		}
		break;

	default:
		break;
	}
}

// ------------------------------------------------------------------- movement

bool AEOGuardCharacter::MoveToward(const FVector& TargetLocation, float DeltaSeconds, float AcceptRadius)
{
	const FVector ToTarget = TargetLocation - GetActorLocation();
	const FVector Flat(ToTarget.X, ToTarget.Y, 0.f);

	const bool bArrived = Flat.SizeSquared() <= AcceptRadius * AcceptRadius;

	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		// No controller to path with: steer directly, as this always used to.
		// Reachable in the editor preview and before possession settles.
		if (!bArrived)
		{
			AddMovementInput(Flat.GetSafeNormal());
		}
		return bArrived;
	}

	if (bArrived)
	{
		AI->StopMovement();
		MoveGoal = FAISystem::InvalidLocation;
		return true;
	}

	// Only ask for a path when the destination actually moves. Reissuing every
	// frame throws away the path being followed and requests another, which both
	// costs a query per frame and makes the guard stutter on the spot.
	if (!FAISystem::IsValidLocation(MoveGoal)
		|| FVector::DistSquared(MoveGoal, TargetLocation) > FMath::Square(MoveGoalTolerance))
	{
		MoveGoal = TargetLocation;

		const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(
			TargetLocation, AcceptRadius, /*bStopOnOverlap=*/true, /*bUsePathfinding=*/true);

		// A guard that cannot path stands still and says nothing, which reads as
		// a broken encounter rather than a missing navmesh. Say it once.
		if (Result == EPathFollowingRequestResult::Failed && !bWarnedPathFailure)
		{
			bWarnedPathFailure = true;
			UE_LOG(LogExecutiveOps, Warning,
				TEXT("Guard %s could not path to %s - is there a navmesh over the arena?"),
				*GetName(), *TargetLocation.ToString());
		}
	}

	return false;
}

bool AEOGuardCharacter::IsDrivenByStateTree() const
{
	const AEOGuardAIController* AI = Cast<AEOGuardAIController>(GetController());
	return AI && AI->IsRunningStateTree();
}

void AEOGuardCharacter::TickPatrolMovement(float DeltaSeconds)
{
	if (PatrolOffsets.Num() == 0)
	{
		return;
	}

	if (PatrolWaitRemaining > 0.f)
	{
		PatrolWaitRemaining -= DeltaSeconds;
		return;
	}

	const FVector Waypoint = PatrolOrigin + PatrolOffsets[PatrolIndex % PatrolOffsets.Num()];
	if (MoveToward(Waypoint, DeltaSeconds, 90.f))
	{
		PatrolIndex = (PatrolIndex + 1) % PatrolOffsets.Num();
		PatrolWaitRemaining = PatrolWaitTime;
	}
}

void AEOGuardCharacter::TickPursueMovement(float DeltaSeconds)
{
	if (!Target)
	{
		return;
	}

	const FVector Pursue = GetPursuitLocation();
	const FVector ToTarget = Pursue - GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}

	// Close to a useful range and hold there: the guard should stay dangerous
	// without walking into the operative's knife.
	if (ToTarget.Size2D() > PreferredCombatRange)
	{
		MoveToward(Pursue, DeltaSeconds, PreferredCombatRange);
	}
}

bool AEOGuardCharacter::TickSearchMovement(float DeltaSeconds)
{
	if (MoveToward(LastKnownTargetLocation, DeltaSeconds, 120.f))
	{
		// Arrived and found nothing: look around rather than standing still.
		AddActorWorldRotation(FRotator(0.f, 60.f * DeltaSeconds, 0.f));
	}

	SearchRemaining -= DeltaSeconds;
	return SearchRemaining <= 0.f;
}

void AEOGuardCharacter::TickEngagement(float DeltaSeconds)
{
	if (bTargetVisible && Weapon->IsReady() && FirstShotRemaining <= 0.f)
	{
		FireAtTarget();
	}

	FirstShotRemaining = FMath::Max(FirstShotRemaining - DeltaSeconds, 0.f);
}

void AEOGuardCharacter::UpdateMovement(float DeltaSeconds)
{
	switch (State)
	{
	case EEOGuardState::Patrolling:
	{
		if (PatrolOffsets.Num() == 0)
		{
			return;
		}

		if (PatrolWaitRemaining > 0.f)
		{
			PatrolWaitRemaining -= DeltaSeconds;
			return;
		}

		const FVector Waypoint = PatrolOrigin + PatrolOffsets[PatrolIndex % PatrolOffsets.Num()];
		if (MoveToward(Waypoint, DeltaSeconds, 90.f))
		{
			PatrolIndex = (PatrolIndex + 1) % PatrolOffsets.Num();
			PatrolWaitRemaining = PatrolWaitTime;
		}
		break;
	}

	case EEOGuardState::Suspicious:
		// Stop and look. Committing to a move on a half-sighting is what makes a
		// guard feel twitchy rather than watchful.
		if (Target)
		{
			const FVector Look = GetPursuitLocation() - GetActorLocation();
			if (!Look.IsNearlyZero())
			{
				SetActorRotation(FRotator(0.f, Look.Rotation().Yaw, 0.f));
			}
		}
		break;

	case EEOGuardState::Alerted:
	{
		if (!Target)
		{
			break;
		}

		const FVector Pursue = GetPursuitLocation();
		const FVector ToTarget = Pursue - GetActorLocation();
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}

		// Close to a useful range and hold there: the guard should stay dangerous
		// without walking into the operative's knife.
		if (ToTarget.Size2D() > PreferredCombatRange)
		{
			MoveToward(Pursue, DeltaSeconds, PreferredCombatRange);
		}
		break;
	}

	case EEOGuardState::Searching:
		if (MoveToward(LastKnownTargetLocation, DeltaSeconds, 120.f))
		{
			// Arrived and found nothing: look around rather than standing still.
			AddActorWorldRotation(FRotator(0.f, 60.f * DeltaSeconds, 0.f));
		}
		break;

	default:
		break;
	}
}

// -------------------------------------------------------------------- weapon

void AEOGuardCharacter::FireAtTarget()
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return;
	}

	const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, 50.f) + GetActorForwardVector() * 40.f;

	// The guard aims straight at the target. It has no camera, so there is no
	// second stage the way the operative has one - the aim point is the target.
	const FEOShotResult Shot = Weapon->TryFire(Muzzle, Target->GetActorLocation());
	if (!Shot.bFired)
	{
		return;
	}

	if (!Shot.bHitDamageable)
	{
		// A miss is the interesting case. The brief's test for this verb is that a
		// near miss moves the player before any health is lost, so the whizz is
		// placed at the closest point of the shot line to them - i.e. where it
		// actually passed - and scaled by how close that was.
		UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this);
		if (!Feedback)
		{
			return;
		}

		const FVector MuzzleToTarget = Target->GetActorLocation() - Muzzle;
		const float Along = FMath::Clamp(
			FVector::DotProduct(MuzzleToTarget, Shot.Direction), 0.f, Weapon->Range);
		const FVector ClosestPoint = Muzzle + Shot.Direction * Along;
		const float MissDistance = FVector::Dist(ClosestPoint, Target->GetActorLocation());

		if (MissDistance < NearMissRadius)
		{
			FEOFeedbackContext Whizz = FEOFeedbackContext::At(ClosestPoint);
			Whizz.Rotation = Shot.Direction.Rotation();
			Whizz.Scale = FMath::Clamp(1.f - MissDistance / NearMissRadius, 0.3f, 1.f);
			Feedback->Play(EOFeedbackEvents::Guard_NearMiss, Whizz);
		}

		if (Shot.bBlockingHit)
		{
			// Something took the round. Dust or sparks nearby is half of why enemy
			// fire reads as dangerous rather than as an abstract health drain.
			FEOFeedbackContext Impact = FEOFeedbackContext::At(Shot.Hit.ImpactPoint);
			Impact.Rotation = Shot.Hit.ImpactNormal.Rotation();
			Feedback->Play(EOFeedbackEvents::Pistol_HitHard, Impact);
		}
	}

	if (FireAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->PlayAnimation(FireAnim, false);
			CurrentAnim = FireAnim;
		}
	}
}

// ----------------------------------------------------------------- takedown

bool AEOGuardCharacter::CanBeTakenDownBy(const AActor* Attacker) const
{
	if (!Attacker || IsDead())
	{
		return false;
	}

	// An alerted guard is looking at you. Silent kills are the reward for not
	// having been seen, so this is where stealth stops paying out.
	if (State == EEOGuardState::Alerted)
	{
		return false;
	}

	const FVector ToAttacker = Attacker->GetActorLocation() - GetActorLocation();

	// Horizontal reach with a separate vertical limit. A single 3D distance test
	// lets the player knife the guard from a walkway directly overhead.
	if (ToAttacker.Size2D() > TakedownRange ||
		FMath::Abs(ToAttacker.Z) > TakedownMaxHeightDifference)
	{
		return false;
	}

	// No reaching through walls.
	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EOGuardTakedown), false, this);
		Params.AddIgnoredActor(Attacker);

		FHitResult Blocking;
		if (World->LineTraceSingleByChannel(Blocking,
			GetActorLocation(), Attacker->GetActorLocation(), ECC_Visibility, Params))
		{
			return false;
		}
	}

	// Measured from the guard's back: the attacker has to be behind them.
	const float Angle = FMath::RadiansToDegrees(
		FMath::Acos(FVector::DotProduct(-GetActorForwardVector(), ToAttacker.GetSafeNormal2D())));

	return Angle <= TakedownRearAngle * 0.5f;
}

void AEOGuardCharacter::Takedown(AActor* Attacker)
{
	if (!Health || Health->IsDead())
	{
		return;
	}

	// Lethal by definition, so more than enough damage to finish the job whatever
	// the guard's health. The damage type is what makes this a takedown rather
	// than a shot, and it is why HandleDamaged below does not react to it.
	UGameplayStatics::ApplyDamage(this, Health->GetMaxHealth() * 2.f,
		Attacker ? Attacker->GetInstigatorController() : nullptr, Attacker,
		UEOTakedownDamageType::StaticClass());
}

void AEOGuardCharacter::HandleDied(AActor* Killer)
{
	SetState(EEOGuardState::Dead);

	UE_LOG(LogExecutiveOps, Log, TEXT("Guard %s killed by %s."), *GetName(), *GetNameSafe(Killer));

	// The body stays where it fell. Nothing else has to react to it, because
	// there is nothing else.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// Capsule stops blocking so the body is not an invisible wall, but the mesh
	// stays visible lying where it died.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The mesh keeps blocking Visibility otherwise, so the body would soak up
	// shots and block sight lines.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (DeathAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->PlayAnimation(DeathAnim, false);
			CurrentAnim = DeathAnim;
		}
	}
}

void AEOGuardCharacter::ResetGuard()
{
	SetActorLocationAndRotation(PatrolOrigin, PatrolOriginRotation,
		/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	State = EEOGuardState::Patrolling;
	Target = nullptr;
	CurrentAnim = nullptr;

	PatrolIndex = 0;
	PatrolWaitRemaining = 0.f;
	DetectionAlpha = 0.f;
	TimeSinceSeen = 0.f;
	SearchRemaining = 0.f;
	FirstShotRemaining = 0.f;
	Weapon->ResetWeapon();
	bTargetVisible = false;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
		Movement->MaxWalkSpeed = PatrolSpeed;
		Movement->bOrientRotationToMovement = true;
	}

	if (Health)
	{
		Health->Revive();
	}
}

// ------------------------------------------------------------------ animation

void AEOGuardCharacter::UpdateAnimation()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// The death pose must not be overwritten by locomotion selection.
	if (State == EEOGuardState::Dead)
	{
		return;
	}

	UAnimSequence* Wanted = nullptr;

	if (State == EEOGuardState::Alerted)
	{
		Wanted = AimAnim;
	}
	else if (GetVelocity().Size2D() > 10.f)
	{
		Wanted = WalkAnim;
	}
	else
	{
		Wanted = IdleAnim;
	}

	// The fire clip is a one-shot; let it finish before locomotion takes over.
	if (CurrentAnim == FireAnim && FireAnim && MeshComp->IsPlaying())
	{
		return;
	}

	if (Wanted && Wanted != CurrentAnim)
	{
		MeshComp->PlayAnimation(Wanted, true);
		CurrentAnim = Wanted;
	}
}

// ----------------------------------------------------------------------- tick

void AEOGuardCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == EEOGuardState::Dead)
	{
		return;
	}

	// Perception always runs: the tree reads what the guard knows, it does not
	// replace how the guard comes to know it.
	UpdatePerception(DeltaSeconds);

	// Deciding and acting belong to the tree when there is one. Assigning a tree
	// is then the whole switch, and removing it is the whole revert.
	if (!IsDrivenByStateTree())
	{
		UpdateState(DeltaSeconds);
		UpdateMovement(DeltaSeconds);
	}

	// The weapon owns the rate of fire and counts its own cooldown down. The
	// guard used to keep a second copy of both, which meant two numbers that had
	// to agree.
	if (!IsDrivenByStateTree()
		&& State == EEOGuardState::Alerted && bTargetVisible && Weapon->IsReady()
		&& FirstShotRemaining <= 0.f)
	{
		FireAtTarget();
	}

	if (!IsDrivenByStateTree())
	{
		FirstShotRemaining = FMath::Max(FirstShotRemaining - DeltaSeconds, 0.f);
	}

	UpdateAnimation();
}
