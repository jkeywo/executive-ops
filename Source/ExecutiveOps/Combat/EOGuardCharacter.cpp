#include "Combat/EOGuardCharacter.h"

#include "Animation/AnimSequence.h"
#include "Character/EOOperativeCharacter.h"
#include "Combat/EOHealthComponent.h"
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

	// Without a controller the movement component never runs, so a guard spawned
	// at runtime would neither move nor fall, silently.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// One guard, one possible target: the operative. A perception system that
	// enumerated stimuli would be the "final alert architecture" M5 says to skip.
	for (TActorIterator<AEOOperativeCharacter> It(World); It; ++It)
	{
		AEOOperativeCharacter* Operative = *It;

		// An operative that is stowed or still falling out of the aircraft is not
		// in the world as far as the guard is concerned.
		if (!Operative || Operative->IsStowed())
		{
			continue;
		}

		if (const UEOHealthComponent* TargetHealth = Operative->GetHealth())
		{
			if (TargetHealth->IsDead())
			{
				continue;
			}
		}

		const FVector EyeLocation = GetActorLocation() + FVector(0.f, 0.f, EyeHeight);
		const FVector ToTarget = Operative->GetActorLocation() - EyeLocation;
		const float Distance = ToTarget.Size();

		if (Distance > SightRange)
		{
			continue;
		}

		// Close enough and the guard notices regardless of where they are looking:
		// walking straight into someone's back should not be free.
		if (Distance > ProximityRange)
		{
			// The cone is judged horizontally. The guard never pitches, so
			// measuring in 3D would spend the whole cone budget on height and
			// blind it to anyone standing on a walkway in front of it.
			const FVector FlatToTarget = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
			if (!FlatToTarget.IsNearlyZero())
			{
				const float Angle = FMath::RadiansToDegrees(FMath::Acos(
					FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), FlatToTarget)));
				if (Angle > SightHalfAngle)
				{
					continue;
				}
			}
		}

		if (!HasLineOfSightTo(*Operative))
		{
			continue;
		}

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

void AEOGuardCharacter::HandleDamaged(float Amount, AActor* DamageInstigator)
{
	if (State == EEOGuardState::Dead)
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
		// player moves immediately.
		FireCooldown = FMath::Max(FireCooldown, FirstShotDelay);
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

	if (Flat.SizeSquared() <= AcceptRadius * AcceptRadius)
	{
		return true;
	}

	AddMovementInput(Flat.GetSafeNormal());
	return false;
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
	const FVector ToTarget = (Target->GetActorLocation() - Muzzle).GetSafeNormal();

	// Spread, so a moving target is genuinely harder to hit and the player is
	// rewarded for not standing still.
	const FVector Direction = FMath::VRandCone(ToTarget, FMath::DegreesToRadians(ShotSpread));
	const FVector End = Muzzle + Direction * SightRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOGuardShot), false, this);

	// ECC_Pawn, not ECC_Visibility: the default Pawn collision profile ignores
	// the Visibility channel, so a Visibility trace goes straight through the
	// character it is aimed at and buries itself in the scenery behind them.
	UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this);
	if (Feedback)
	{
		FEOFeedbackContext Shot = FEOFeedbackContext::At(Muzzle);
		Shot.Rotation = Direction.Rotation();
		Feedback->Play(EOFeedbackEvents::Guard_Fire, Shot);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Muzzle, End, ECC_Pawn, Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	bool bHitTarget = false;

	if (HitActor)
	{
		if (UEOHealthComponent* HitHealth = HitActor->FindComponentByClass<UEOHealthComponent>())
		{
			HitHealth->ApplyDamage(ShotDamage, this);
			bHitTarget = true;
		}
	}

	if (Feedback && !bHitTarget)
	{
		// A miss is the interesting case. The brief's test for this verb is that a
		// near miss moves the player before any health is lost, so the whizz is
		// placed at the closest point of the shot line to them - i.e. where it
		// actually passed - and scaled by how close that was.
		const FVector MuzzleToTarget = Target->GetActorLocation() - Muzzle;
		const float Along = FMath::Clamp(FVector::DotProduct(MuzzleToTarget, Direction), 0.f, SightRange);
		const FVector ClosestPoint = Muzzle + Direction * Along;
		const float MissDistance = FVector::Dist(ClosestPoint, Target->GetActorLocation());

		if (MissDistance < NearMissRadius)
		{
			FEOFeedbackContext Whizz = FEOFeedbackContext::At(ClosestPoint);
			Whizz.Rotation = Direction.Rotation();
			Whizz.Scale = FMath::Clamp(1.f - MissDistance / NearMissRadius, 0.3f, 1.f);
			Feedback->Play(EOFeedbackEvents::Guard_NearMiss, Whizz);
		}

		if (bHit)
		{
			// Something took the round. Dust or sparks nearby is half of why enemy
			// fire reads as dangerous rather than as an abstract health drain.
			FEOFeedbackContext Impact = FEOFeedbackContext::At(Hit.ImpactPoint);
			Impact.Rotation = Hit.ImpactNormal.Rotation();
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
	if (Health)
	{
		Health->Kill(Attacker);
	}
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
	FireCooldown = 0.f;
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

	UpdatePerception(DeltaSeconds);
	UpdateState(DeltaSeconds);
	UpdateMovement(DeltaSeconds);

	FireCooldown = FMath::Max(FireCooldown - DeltaSeconds, 0.f);
	if (State == EEOGuardState::Alerted && bTargetVisible && FireCooldown <= 0.f)
	{
		FireAtTarget();
		FireCooldown = FireInterval;
	}

	UpdateAnimation();
}
