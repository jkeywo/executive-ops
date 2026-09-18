#include "Tests/EOTraversalTest.h"

#include "ExecutiveOpsDev.h"

#include "Character/EOOperativeCharacter.h"
#include "Character/EOTraversalComponent.h"
#include "Core/EOPlayerController.h"
#include "Interfaces/EODeployableInterface.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	// The obstacles, as m0_setup.py builds them, and a stride in front of each.
	const FVector LowWallApproach(1625.f, 0.f, 96.f);
	const FVector LedgeApproach(4350.f, 0.f, 96.f);
	const FVector TallWallApproach(5950.f, -300.f, 96.f);
	const FVector OpenGround(600.f, 0.f, 96.f);
}

AEOTraversalTest::AEOTraversalTest()
{
	TimeLimit = 60.f;
}

bool AEOTraversalTest::CheckTraversalAt(const FVector& StandLocation, float FacingYaw,
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

void AEOTraversalTest::CheckTraversalEnded(const TCHAR* Verb)
{
	AEOOperativeCharacter* Op = GetOperative();
	if (!Op || !Op->GetTraversal())
	{
		return;
	}

	Check(!Op->GetTraversal()->IsTraversing(), FString::Printf(TEXT("%s completes"), Verb));

	// Collision left disabled would be catastrophic and invisible.
	Check(Op->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision,
		FString::Printf(TEXT("collision is restored after a %s"), Verb));
}

void AEOTraversalTest::Step()
{
	AEOPlayerController* Controller = GetController();
	if (!Controller)
	{
		Check(false, TEXT("no player controller"));
		Done();
		return;
	}

	switch (Current())
	{
	case EPhase::Setup:
	{
		Check(Controller->PossessOperative(), TEXT("can possess the operative"));

		AEOOperativeCharacter* Op = GetOperative();
		Check(Op != nullptr, TEXT("operative is possessed"));
		if (!Op)
		{
			Done();
			return;
		}

		IEODeployableInterface::Execute_SetStowed(Op, false);
		Check(Op->HasControl(), TEXT("operative has ground control"));
		Check(Op->GetTraversal() != nullptr, TEXT("operative has a traversal component"));

		// Stand a stride in front of the 90cm wall, facing it.
		CheckTraversalAt(LowWallApproach, 0.f, EEOTraversalType::Vault, TEXT("low wall"));
		Go(EPhase::VaultCheck, 1.0f);
		return;
	}

	case EPhase::VaultCheck:
	{
		CheckTraversalEnded(TEXT("vault"));
		if (AEOOperativeCharacter* Op = GetOperative())
		{
			Check(Op->GetActorLocation().X > 1800.f,
				FString::Printf(TEXT("vault ends past the wall (x=%.0f)"), Op->GetActorLocation().X));
		}

		CheckTraversalAt(LedgeApproach, 0.f, EEOTraversalType::Mantle, TEXT("180cm ledge"));
		Go(EPhase::MantleCheck, 1.4f);
		return;
	}

	case EPhase::MantleCheck:
	{
		CheckTraversalEnded(TEXT("mantle"));
		if (AEOOperativeCharacter* Op = GetOperative())
		{
			Check(Op->GetActorLocation().Z > 200.f,
				FString::Printf(TEXT("mantle ends on top of the ledge (z=%.0f)"), Op->GetActorLocation().Z));
		}

		CheckTraversalAt(TallWallApproach, 0.f, EEOTraversalType::Climb, TEXT("380cm wall"));
		Go(EPhase::ClimbCheck, 1.8f);
		return;
	}

	case EPhase::ClimbCheck:
	{
		CheckTraversalEnded(TEXT("climb"));
		AEOOperativeCharacter* Op = GetOperative();
		if (Op && Op->GetTraversal())
		{
			Check(Op->GetActorLocation().Z > 400.f,
				FString::Printf(TEXT("climb ends on top of the wall (z=%.0f)"), Op->GetActorLocation().Z));

			// Open floor with nothing to traverse must not report a false positive.
			Op->SetActorLocationAndRotation(OpenGround, FRotator::ZeroRotator,
				false, nullptr, ETeleportType::TeleportPhysics);
			Check(!Op->GetTraversal()->Scan().IsValid(),
				TEXT("open ground reports nothing to traverse"));
		}

		Go(EPhase::StowInterrupt, 0.5f);
		return;
	}

	case EPhase::StowInterrupt:
	{
		AEOOperativeCharacter* Op = GetOperative();
		// Stowing mid-traversal must not leave collision on: the order of cancel
		// and disable decides whether a hidden operative blocks the world.
		if (Op && Op->GetTraversal())
		{
			CheckTraversalAt(LowWallApproach, 0.f, EEOTraversalType::Vault, TEXT("stow-interrupt vault"));

			IEODeployableInterface::Execute_SetStowed(Op, true);
			Check(!Op->GetTraversal()->IsTraversing(), TEXT("stowing cancels a traversal"));
			Check(!Op->GetActorEnableCollision(),
				TEXT("a stowed operative has no collision, even mid-traversal"));

			IEODeployableInterface::Execute_SetStowed(Op, false);
			Check(Op->GetActorEnableCollision(), TEXT("unstowing restores collision"));
		}

		Go(EPhase::SlideCheck, 0.5f);
		return;
	}

	case EPhase::SlideCheck:
	{
		AEOOperativeCharacter* Op = GetOperative();
		if (Op)
		{
			Op->SetActorLocationAndRotation(OpenGround, FRotator::ZeroRotator,
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
		Go(EPhase::SlideConfirm, 0.4f);
		return;
	}

	case EPhase::SlideConfirm:
	{
		if (AEOOperativeCharacter* Op = GetOperative())
		{
			Check(Op->bIsCrouched, TEXT("sliding actually crouches the capsule"));

			Op->StopSlide();
			Check(!Op->IsSliding(), TEXT("slide can be ended"));

			if (UCharacterMovementComponent* Movement = Op->GetCharacterMovement())
			{
				Check(FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 500.f, 1.f),
					FString::Printf(TEXT("walk speed restored after a slide (%.0f)"), Movement->MaxWalkSpeed));
			}
		}

		Go(EPhase::MeshDrift, 0.2f);
		return;
	}

	case EPhase::MeshDrift:
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
			// nothing switches it back. Left alone, this check quietly disables
			// the Animation Blueprint for everything that follows.
			const EAnimationMode::Type PreviousMode = MeshComp->GetAnimationMode();

			const TArray<UAnimSequence*> Clips = Op->GetLocomotionClips();
			Check(Clips.Num() >= 3, FString::Printf(TEXT("the locomotion clips are assigned (%d)"), Clips.Num()));

			for (UAnimSequence* Clip : Clips)
			{
				MeshComp->PlayAnimation(Clip, false);

				// Most of the way through, where a clip that travels has travelled.
				MeshComp->SetPosition(Clip->GetPlayLength() * 0.9f, false);
				MeshComp->RefreshBoneTransforms();

				const FVector RootWorld = MeshComp->GetBoneLocation(TEXT("root"), EBoneSpaces::WorldSpace);
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

		Go(EPhase::Finished, 0.2f);
		return;
	}

	case EPhase::Finished:
	default:
		Done();
		return;
	}
}
