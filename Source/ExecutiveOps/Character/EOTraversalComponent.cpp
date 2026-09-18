#include "Character/EOTraversalComponent.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Character/EOOperativeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ExecutiveOps.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** A surface flatter than this is not something to stand on. */
	constexpr float MinWalkableNormalZ = 0.7f;

	/** A face steeper than this is a wall; anything flatter is a ramp to run up. */
	constexpr float MaxFaceNormalZ = 0.4f;
}

UEOTraversalComponent::UEOTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEOTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEOTraversalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Collision and movement mode are restored by Cancel. Without this, a
	// traversal interrupted by teardown or deactivation would leave the capsule
	// permanently non-colliding and stuck in MOVE_Flying.
	Cancel();
	Super::EndPlay(EndPlayReason);
}

void UEOTraversalComponent::Deactivate()
{
	Cancel();
	Super::Deactivate();
}

ACharacter* UEOTraversalComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

bool UEOTraversalComponent::IsTraversalAllowed() const
{
	// Never start a traversal for a pawn that is stowed, mid-drop or unpossessed:
	// a buffered press would otherwise fly an idle operative across the level.
	const AEOOperativeCharacter* Operative = Cast<AEOOperativeCharacter>(GetOwner());
	if (Operative && !Operative->HasControl())
	{
		return false;
	}

	const ACharacter* Character = GetCharacter();
	return Character && Character->GetController() != nullptr;
}

float UEOTraversalComponent::DurationFor(EEOTraversalType Type) const
{
	switch (Type)
	{
	case EEOTraversalType::Vault:	return VaultDuration;
	case EEOTraversalType::Mantle:	return MantleDuration;
	case EEOTraversalType::Climb:	return ClimbDuration;
	default:						return 0.f;
	}
}

UAnimMontage* UEOTraversalComponent::MontageFor(EEOTraversalType Type) const
{
	switch (Type)
	{
	case EEOTraversalType::Vault:	return VaultMontage;
	case EEOTraversalType::Mantle:	return MantleMontage;
	case EEOTraversalType::Climb:	return ClimbMontage;
	default:						return nullptr;
	}
}

UAnimSequence* UEOTraversalComponent::AnimationFor(EEOTraversalType Type) const
{
	switch (Type)
	{
	case EEOTraversalType::Vault:	return VaultAnim;
	case EEOTraversalType::Mantle:	return MantleAnim;
	case EEOTraversalType::Climb:	return ClimbAnim;
	default:						return nullptr;
	}
}

void UEOTraversalComponent::BufferInput()
{
	BufferRemaining = InputBufferTime;
}

bool UEOTraversalComponent::FindGround(const FVector& From, float Depth,
	const AActor* RequiredActor, FVector& OutGround) const
{
	const ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOTraversalGround), false, Character);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(
		Hit, From, From - FVector(0.f, 0.f, Depth), ECC_Pawn, Params))
	{
		return false;
	}

	// A trace that begins inside geometry reports its own start as the impact,
	// which would fabricate a surface out of thin air.
	if (Hit.bStartPenetrating)
	{
		return false;
	}

	// Only flat-enough surfaces count. The side of a pillar or a pitched roof is
	// not a ledge, and treating it as one drops the player straight back off.
	if (Hit.ImpactNormal.Z < MinWalkableNormalZ)
	{
		return false;
	}

	// The top must belong to the thing that was actually hit. Without this the
	// downward probe happily lands on a ceiling, an awning or a bridge overhead
	// and the player is flown up through it.
	if (RequiredActor && Hit.GetActor() != RequiredActor)
	{
		return false;
	}

	OutGround = Hit.ImpactPoint;
	return true;
}

bool UEOTraversalComponent::IsCapsuleClear(const FVector& AtLocation) const
{
	const ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOTraversalClear), false, Character);

	return !GetWorld()->OverlapBlockingTestByChannel(
		AtLocation, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(
			Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleHalfHeight()),
		Params);
}

FEOTraversalQuery UEOTraversalComponent::Scan() const
{
	FEOTraversalQuery Result;

	const ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return Result;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

	// The destination is measured against the STANDING capsule: a traversal that
	// finishes while crouched will stand up, and an end position computed from
	// the crouched height would bury the character in the floor.
	const float HalfHeight = Character->GetDefaultHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();

	const FVector Origin = Character->GetActorLocation();
	const FVector FeetLocation = Origin - FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight());
	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOTraversalScan), false, Character);

	// Probe low, so the same sweep finds a knee-high crate and a chest-high wall.
	const FVector ProbeStart = FeetLocation + FVector(0.f, 0.f, MinObstacleHeight + ScanRadius);
	const FVector ProbeEnd = ProbeStart + Forward * ScanDistance;

	FHitResult FaceHit;
	if (!GetWorld()->SweepSingleByChannel(FaceHit, ProbeStart, ProbeEnd, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(ScanRadius), Params))
	{
		return Result;
	}

	// Steep faces only. A ramp is something to run up, not something to vault.
	if (FMath::Abs(FaceHit.ImpactNormal.Z) > MaxFaceNormalZ)
	{
		return Result;
	}

	const AActor* Obstacle = FaceHit.GetActor();
	const FVector FaceXY(FaceHit.ImpactPoint.X, FaceHit.ImpactPoint.Y, 0.f);
	const float ProbeTopZ = FeetLocation.Z + ClimbMaxHeight + 50.f;

	// Sample a few depths past the face. A single fixed offset misses the top of
	// anything thinner than that offset - railings, fences, thin walls - which
	// reads to the player as the detection being unreliable on exactly the props
	// it should be most generous with.
	static const float DepthFactors[] = { 0.25f, 0.6f, 1.2f, 2.5f };

	FVector TopPoint;
	bool bFoundTop = false;
	for (const float Factor : DepthFactors)
	{
		const FVector ProbeFrom = FaceXY + Forward * (Radius * Factor) + FVector(0.f, 0.f, ProbeTopZ);
		if (FindGround(ProbeFrom, ClimbMaxHeight + 100.f, Obstacle, TopPoint))
		{
			bFoundTop = true;
			break;
		}
	}

	if (!bFoundTop)
	{
		return Result;
	}

	const float Height = TopPoint.Z - FeetLocation.Z;
	if (Height < MinObstacleHeight || Height > ClimbMaxHeight)
	{
		return Result;
	}

	Result.ObstacleHeight = Height;

	// A low obstacle with clear ground beyond is vaulted: the operative keeps
	// going rather than stopping on top of a crate.
	if (Height <= VaultMaxHeight)
	{
		const FVector BeyondStart = TopPoint + Forward * VaultLandingProbe + FVector(0.f, 0.f, 40.f);

		FVector Landing;
		if (FindGround(BeyondStart, MaxVaultDrop + 100.f, nullptr, Landing))
		{
			// Refuse a vault that turns into a plunge. Clearing a parapet at a
			// rooftop edge should not fly the player six metres down through
			// whatever happens to be in the way.
			const bool bReasonableDrop = (TopPoint.Z - Landing.Z) <= MaxVaultDrop;

			const FVector LandingCapsule = Landing + FVector(0.f, 0.f, HalfHeight + 5.f);
			if (bReasonableDrop && IsCapsuleClear(LandingCapsule))
			{
				const FVector Apex = TopPoint + FVector(0.f, 0.f, HalfHeight + 30.f);

				// The arc must not pass through anything on the way over.
				if (IsPathClear(Character->GetActorLocation(), Apex, LandingCapsule))
				{
					Result.Type = EEOTraversalType::Vault;
					Result.EndLocation = LandingCapsule;
					Result.ApexLocation = Apex;
					return Result;
				}
			}
		}
	}

	// Otherwise end up standing on it. Which animation plays is just a question
	// of how far up the operative has to haul themselves.
	// Land just past the lip, and test clearance where the capsule actually ends
	// up rather than somewhere near it.
	const FVector EndLocation = TopPoint
		+ Forward * (Radius + 5.f)
		+ FVector(0.f, 0.f, HalfHeight + 5.f);

	if (!IsCapsuleClear(EndLocation))
	{
		return Result;
	}

	// The top has to be deep enough to stand on: a ledge shallower than the
	// capsule would drop the player off the far side the moment they arrive.
	FVector StandGround;
	if (!FindGround(EndLocation + FVector(0.f, 0.f, 10.f), HalfHeight + 60.f, Obstacle, StandGround))
	{
		return Result;
	}

	Result.Type = (Height <= MantleMaxHeight) ? EEOTraversalType::Mantle : EEOTraversalType::Climb;
	Result.EndLocation = EndLocation;
	Result.ApexLocation = TopPoint + FVector(0.f, 0.f, HalfHeight + 20.f);
	return Result;
}

bool UEOTraversalComponent::IsPathClear(const FVector& Start, const FVector& Apex, const FVector& End) const
{
	const ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOTraversalPath), false, Character);

	// Sweep a slim probe along the arc. The obstacle being vaulted is expected to
	// be underneath it, so this catches walls and ceilings rather than the thing
	// being traversed.
	const FCollisionShape Probe = FCollisionShape::MakeSphere(Capsule->GetScaledCapsuleRadius() * 0.6f);

	const FVector Control = ControlPointFor(Start, Apex, End);

	constexpr int32 Segments = 6;
	FVector Previous = Start;
	for (int32 i = 1; i <= Segments; ++i)
	{
		const float T = static_cast<float>(i) / Segments;
		const FVector Point = EvaluateArc(Start, Control, End, T);

		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Previous, Point, FQuat::Identity,
			ECC_Pawn, Probe, Params))
		{
			return false;
		}
		Previous = Point;
	}

	return true;
}

FVector UEOTraversalComponent::ControlPointFor(const FVector& Start, const FVector& Apex, const FVector& End)
{
	// A quadratic Bezier only travels halfway to its control point, so feeding it
	// the desired apex directly leaves the curve well short - low enough that a
	// vault passes through the obstacle it is supposed to clear. Solving
	// B(0.5) == Apex gives this control point instead.
	return 2.f * Apex - (Start + End) * 0.5f;
}

FVector UEOTraversalComponent::EvaluateArc(const FVector& Start, const FVector& Control,
	const FVector& End, float T)
{
	const float Inv = 1.f - T;
	return Inv * Inv * Start + 2.f * Inv * T * Control + T * T * End;
}

bool UEOTraversalComponent::TryTraverse()
{
	if (!IsTraversalAllowed())
	{
		return false;
	}

	if (bTraversing)
	{
		// Keep the press: chaining two obstacles back to back should not require
		// frame-perfect timing on the second one.
		BufferInput();
		return false;
	}

	const FEOTraversalQuery Query = Scan();
	if (!Query.IsValid())
	{
		// Nothing there yet: remember the press so closing the last stride still works.
		BufferInput();
		return false;
	}

	Begin(Query);
	return true;
}

void UEOTraversalComponent::Begin(const FEOTraversalQuery& Query)
{
	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}

	// A slide in progress would keep writing velocity and uncrouch the capsule
	// halfway through the arc.
	if (AEOOperativeCharacter* Operative = Cast<AEOOperativeCharacter>(Character))
	{
		Operative->StopSlide();
	}

	// Commitment is the phase the player most needs confirmed - the approach is
	// theirs, the contact is the animation's, but the moment the system accepted
	// the press is only knowable if something says so.
	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		FEOFeedbackContext Context = FEOFeedbackContext::AtActor(Character);
		Context.AttachTo = Character->GetRootComponent();

		switch (Query.Type)
		{
		case EEOTraversalType::Vault:	Feedback->Play(EOFeedbackEvents::Parkour_Vault, Context); break;
		case EEOTraversalType::Mantle:	Feedback->Play(EOFeedbackEvents::Parkour_Mantle, Context); break;
		case EEOTraversalType::Climb:	Feedback->Play(EOFeedbackEvents::Parkour_Climb, Context); break;
		default: break;
		}

		// Hand or foot meeting the obstacle, at the obstacle rather than at the
		// character: the sound belongs to the surface being touched.
		FEOFeedbackContext Contact = FEOFeedbackContext::At(Query.ApexLocation);
		Feedback->Play(EOFeedbackEvents::Parkour_Contact, Contact);
	}

	ActiveQuery = Query;
	StartLocation = Character->GetActorLocation();
	ControlPoint = ControlPointFor(StartLocation, Query.ApexLocation, Query.EndLocation);
	Duration = FMath::Max(DurationFor(Query.Type), 0.05f);
	Elapsed = 0.f;
	bTraversing = true;
	BufferRemaining = 0.f;

	// The motion is fully authored by the interpolation below, so the movement
	// component must not also be trying to walk, fall or act on player input.
	// MOVE_None rather than MOVE_Flying: flying would accelerate on held input
	// and fight the transform this component is writing every frame.
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);
	}
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Routed through the character so a traversal uses the same montage-or-clip
	// rule as its other one-shots, rather than inventing a second one.
	if (AEOOperativeCharacter* Operative = Cast<AEOOperativeCharacter>(Character))
	{
		Operative->PlayActionAnimation(MontageFor(Query.Type), AnimationFor(Query.Type));
	}

	UE_LOG(LogExecutiveOps, Verbose, TEXT("Traversal: %s over %.0fcm"),
		*StaticEnum<EEOTraversalType>()->GetNameStringByValue(static_cast<int64>(Query.Type)),
		Query.ObstacleHeight);
}

void UEOTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (BufferRemaining > 0.f)
	{
		BufferRemaining = FMath::Max(BufferRemaining - DeltaTime, 0.f);

		// A buffered press fires the moment an obstacle comes into range.
		if (!bTraversing && IsTraversalAllowed())
		{
			const FEOTraversalQuery Query = Scan();
			if (Query.IsValid())
			{
				Begin(Query);
			}
		}
	}

	if (!bTraversing)
	{
		return;
	}

	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		Cancel();
		return;
	}

	Elapsed += DeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

	Character->SetActorLocation(
		EvaluateArc(StartLocation, ControlPoint, ActiveQuery.EndLocation, Alpha),
		/*bSweep=*/false);

	if (Alpha >= 1.f)
	{
		Finish();
	}
}

void UEOTraversalComponent::Finish()
{
	bTraversing = false;
	ActiveQuery = FEOTraversalQuery();
	BufferRemaining = 0.f;

	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		// Falling rather than walking: landing on the far side of a vault should
		// resolve through the normal landing path, not snap to the floor.
		Movement->SetMovementMode(MOVE_Falling);
	}
}

void UEOTraversalComponent::Cancel()
{
	if (bTraversing)
	{
		Finish();
	}
	BufferRemaining = 0.f;
}
