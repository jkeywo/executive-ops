#include "Character/EOOperativeCharacter.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Character/EOTraversalComponent.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Combat/EOWeaponComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/EOInteractableInterface.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "ExecutiveOps.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/SpringArmComponent.h"
#include "Core/EOPlayerController.h"
#include "Input/EOInputConfig.h"
#include "Input/EOInputSettings.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOMissionSubsystem.h"
#include "TimerManager.h"

AEOOperativeCharacter::AEOOperativeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// Character faces movement; the camera turns independently.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;

	// Fast enough that a direction change reads as the operative turning, not as
	// the operative deciding whether to. At 640 the body was still swinging round
	// long after the player had committed to the new direction.
	Movement->RotationRate = FRotator(0.f, 1600.f, 0.f);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->JumpZVelocity = 550.f;
	Movement->AirControl = 0.35f;
	Movement->BrakingDecelerationWalking = 2000.f;
	Movement->MaxWalkSpeedCrouched = SlideImpulse;

	// Off by default, which silently makes Crouch() and the whole slide a no-op.
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;

	// Light positional lag only. The boom trails the capsule slightly so footfalls
	// and slides do not transmit straight into the view; rotation is left rigid
	// because the mouse must stay exact.
	//
	// No max distance: a clamp holds the boom at its limit while the operative
	// accelerates away, then releases it all at once when they stop, which reads
	// as the camera snapping back to where it thought you were. Lag alone trails
	// and recovers continuously, so there is nothing to spring.
	//
	// The speed is high enough that a sprint barely drags the view - at 18 the
	// steady-state trail was most of a metre.
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 30.f;
	CameraBoom->CameraLagMaxDistance = 0.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Attached to the body, not the root: the pistol has to ride the animation,
	// which means it lives on a bone whether it is holstered or drawn.
	PistolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PistolMesh"));
	PistolMesh->SetupAttachment(GetMesh(), HolsterSocket);
	PistolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PistolMesh->SetRelativeLocation(HolsterOffset);
	PistolMesh->SetRelativeRotation(HolsterRotation);

	Traversal = CreateDefaultSubobject<UEOTraversalComponent>(TEXT("Traversal"));
	Health = CreateDefaultSubobject<UEOHealthComponent>(TEXT("Health"));

	Weapon = CreateDefaultSubobject<UEOWeaponComponent>(TEXT("Weapon"));
	Weapon->FireEvent = EOFeedbackEvents::Pistol_Fire;

	PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(
		TEXT("PerceptionSource"));
	PerceptionSource->bAutoRegister = true;
	PerceptionSource->RegisterForSense(UAISense_Sight::StaticClass());

	PrimaryActorTick.bCanEverTick = true;
}

void AEOOperativeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Also set at runtime, not just on the CDO: a Blueprint subclass created
	// before this default changed keeps the old serialised value in its
	// component template, which silently makes Crouch() and the slide a no-op.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
		Movement->MaxWalkSpeedCrouched = SlideImpulse;
	}

	if (Health)
	{
		Health->OnDied.AddDynamic(this, &AEOOperativeCharacter::HandleDied);
		Health->OnDamaged.AddDynamic(this, &AEOOperativeCharacter::HandleDamaged);
	}

	// Forced rather than assumed: the constructor attached to the holster, but the
	// offsets may have been retuned on the Blueprint since.
	bWeaponDrawn = true;
	ApplyWeaponAttachment(false);
}

void AEOOperativeCharacter::ApplyWeaponAttachment(bool bDrawn)
{
	if (!PistolMesh || bWeaponDrawn == bDrawn)
	{
		return;
	}

	bWeaponDrawn = bDrawn;

	USkeletalMeshComponent* Body = GetMesh();
	if (!Body)
	{
		return;
	}

	const FName Socket = bDrawn ? GripSocket : HolsterSocket;

	PistolMesh->AttachToComponent(Body,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);

	PistolMesh->SetRelativeLocation(bDrawn ? GripOffset : HolsterOffset);
	PistolMesh->SetRelativeRotation(bDrawn ? GripRotation : HolsterRotation);
}

void AEOOperativeCharacter::UpdateWeaponAttachment(float DeltaSeconds)
{
	HolsterDelayRemaining = FMath::Max(0.f, HolsterDelayRemaining - DeltaSeconds);

	// In the hand while aiming, and for a beat after a shot so a snap shot from
	// the hip does not fire from a holstered weapon and put it straight back.
	// Holstered whenever the operative has no business holding it.
	const bool bWantsDrawn = HasControl()
		&& !IsDead()
		&& !IsPerformingTakedown()
		&& !(Traversal && Traversal->IsTraversing())
		&& !bSliding
		&& (bAiming || HolsterDelayRemaining > 0.f);

	ApplyWeaponAttachment(bWantsDrawn);
}

void AEOOperativeCharacter::HandleDamaged(float Amount, AActor* DamageInstigator,
	const UDamageType* DamageType)
{
	UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this);
	if (!Feedback || !Health)
	{
		return;
	}

	FEOFeedbackContext Context = FEOFeedbackContext::AtActor(this);

	// The indicator needs somewhere to point. Without a known shooter there is
	// nothing honest to draw, so the vignette fires alone.
	if (DamageInstigator)
	{
		Context.SourceLocation = DamageInstigator->GetActorLocation();
		Context.bHasSourceLocation = true;
	}

	// Scales with how much of the remaining health went: the last hit before death
	// should read louder than the first, without a separate "critical" event.
	const float Severity = FMath::Clamp(Amount / FMath::Max(Health->GetMaxHealth() * 0.4f, 1.f), 0.4f, 1.5f);
	Context.Scale = Severity;

	Feedback->Play(EOFeedbackEvents::Player_Damaged, Context);
}

// --------------------------------------------------------------------- combat

bool AEOOperativeCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

AEOGuardCharacter* AEOOperativeCharacter::FindTakedownTarget() const
{
	UWorld* World = GetWorld();
	if (!World || IsDead())
	{
		return nullptr;
	}

	AEOGuardCharacter* Best = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	// The guard owns the rules for whether it can be taken down; this only picks
	// the nearest one that says yes.
	for (TActorIterator<AEOGuardCharacter> It(World); It; ++It)
	{
		AEOGuardCharacter* Guard = *It;
		if (!Guard || !Guard->CanBeTakenDownBy(this))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(Guard->GetActorLocation(), GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			Best = Guard;
		}
	}

	return Best;
}

bool AEOOperativeCharacter::TryTakedown()
{
	if (!HasControl() || IsDead() || IsPerformingTakedown())
	{
		return false;
	}

	AEOGuardCharacter* Victim = FindTakedownTarget();
	if (!Victim)
	{
		return false;
	}

	// Face the kill, so the animation does not play sideways.
	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.f, ToVictim.Rotation().Yaw, 0.f));

	// Acknowledge the input on the frame it was accepted, before the kill
	// resolves: the brief's first question is "did the game take my input".
	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		Feedback->Play(EOFeedbackEvents::Takedown_Commit, FEOFeedbackContext::AtActor(this));

		FEOFeedbackContext Impact = FEOFeedbackContext::AtActor(Victim);
		Impact.Target = this;
		Feedback->Play(EOFeedbackEvents::Takedown_Impact, Impact);
	}

	Victim->Takedown(this);

	TakedownRemaining = TakedownDuration;
	PlayActionAnimation(TakedownMontage, TakedownAnim);

	UE_LOG(LogExecutiveOps, Log, TEXT("Takedown on %s."), *Victim->GetName());
	return true;
}

FGameplayTag AEOOperativeCharacter::SurfaceEventFor(const FHitResult& Hit, bool bHitCharacter)
{
	if (bHitCharacter)
	{
		return EOFeedbackEvents::Pistol_HitCharacter;
	}

	// Physical materials are the right answer and the greybox has none yet, so
	// this falls back to hard surface rather than guessing. Metal is opt-in: a
	// surface has to say it is metal to spark.
	if (const UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get())
	{
		if (PhysMat->GetName().Contains(TEXT("Metal")))
		{
			return EOFeedbackEvents::Pistol_HitMetal;
		}
	}

	return EOFeedbackEvents::Pistol_HitHard;
}

bool AEOOperativeCharacter::FireWeapon()
{
	if (!HasControl() || IsDead() || IsPerformingTakedown() || !Weapon->IsReady())
	{
		return false;
	}

	if (Traversal && Traversal->IsTraversing())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || !FollowCamera)
	{
		return false;
	}

	HolsterDelayRemaining = HolsterDelay;

	// Drawn on the same frame as the shot, so the muzzle effect has a weapon in
	// hand to come from rather than one still on the hip.
	ApplyWeaponAttachment(true);

	// From the weapon when there is one, from the body when the mesh has not been
	// assigned yet - the muzzle flash must not float at the origin.
	Weapon->MuzzleAttachment = (PistolMesh && PistolMesh->GetStaticMesh())
		? static_cast<USceneComponent*>(PistolMesh)
		: static_cast<USceneComponent*>(GetMesh());

	// Two stages, and only the first is the operative's business: the camera
	// decides WHAT is being aimed at, the weapon decides whether the shot gets
	// there. Firing straight from the camera would let the player hit things from
	// behind cover they are fully hidden by, and clip corners their crosshair is
	// nowhere near. The guard has no camera, which is why this half stays here.
	FCollisionQueryParams AimParams(SCENE_QUERY_STAT(EOOperativeAim), false, this);
	const FVector CameraStart = FollowCamera->GetComponentLocation();
	const FVector CameraAim = FollowCamera->GetForwardVector();

	FHitResult CameraHit;
	// ECC_Pawn throughout: the default Pawn profile ignores Visibility, so a
	// Visibility trace passes clean through anything worth shooting.
	const FVector AimPoint = World->LineTraceSingleByChannel(CameraHit, CameraStart,
		CameraStart + CameraAim * Weapon->Range, ECC_Pawn, AimParams)
		? CameraHit.ImpactPoint
		: CameraStart + CameraAim * Weapon->Range;

	const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, 40.f);

	const FEOShotResult Shot = Weapon->TryFire(Muzzle, AimPoint, bAiming);
	if (!Shot.bFired)
	{
		return false;
	}

	if (Shot.bBlockingHit)
	{
		if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
		{
			// What an impact means depends on what was hit, so it stays with the
			// caller rather than inside the weapon.
			FEOFeedbackContext Impact = FEOFeedbackContext::At(Shot.Hit.ImpactPoint);
			Impact.Rotation = Shot.Hit.ImpactNormal.Rotation();
			Impact.Target = Shot.Hit.GetActor();
			Feedback->Play(SurfaceEventFor(Shot.Hit, Shot.bHitDamageable), Impact);
		}
	}

	PlayActionAnimation(FireMontage, FireAnim);

	return true;
}

void AEOOperativeCharacter::HandleDied(AActor* Killer)
{
	UE_LOG(LogExecutiveOps, Log, TEXT("Operative killed by %s."), *GetNameSafe(Killer));

	StopSlide();
	if (Traversal)
	{
		Traversal->Cancel();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	PlayActionAnimation(DeathMontage, DeathAnim);

	// A dead operative is a failed mission. M6 decides what happens next.
	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->FailMission();
	}
}

AActor* AEOOperativeCharacter::FindInteractable() const
{
	UWorld* World = GetWorld();
	if (!World || !HasControl() || IsDead())
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!Candidate || !Candidate->Implements<UEOInteractableInterface>())
		{
			continue;
		}

		// Each interactable decides its own reach, so a wide extraction pad and a
		// terminal you have to stand at can coexist without a magic constant here.
		const float Range = FMath::Min(
			IEOInteractableInterface::Execute_GetInteractionRange(Candidate), MaxInteractionRange);

		const float DistanceSq = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (DistanceSq > Range * Range)
		{
			continue;
		}

		if (!IEOInteractableInterface::Execute_CanInteract(Candidate, const_cast<AEOOperativeCharacter*>(this)))
		{
			continue;
		}

		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			Best = Candidate;
		}
	}

	return Best;
}

bool AEOOperativeCharacter::TryInteract()
{
	// Consistent with the takedown and the traversal: not while the character is
	// already committed to something that owns its transform.
	if (IsPerformingTakedown() || (Traversal && Traversal->IsTraversing()))
	{
		return false;
	}

	AActor* Interactable = FindInteractable();
	if (!Interactable)
	{
		UE_LOG(LogExecutiveOps, Verbose, TEXT("Interact: nothing in range."));
		return false;
	}

	return IEOInteractableInterface::Execute_Interact(Interactable, this);
}

void AEOOperativeCharacter::ResetOperative()
{
	if (Traversal)
	{
		Traversal->Cancel();
	}
	StopSlide();
	UnCrouch();

	bSprinting = false;
	SetAiming(false);
	Weapon->ResetWeapon();
	TakedownRemaining = 0.f;
	CurrentAnim = nullptr;

	if (Health)
	{
		Health->Revive();
	}

	// The death path disabled movement; reviving the health component does not
	// undo that, so without this the operative comes back alive but frozen.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
		Movement->StopMovementImmediately();
		Movement->MaxWalkSpeed = WalkSpeed;
	}

	SetActorEnableCollision(true);
}

void AEOOperativeCharacter::TickCombat(float DeltaSeconds)
{
	TakedownRemaining = FMath::Max(TakedownRemaining - DeltaSeconds, 0.f);
}

void AEOOperativeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickCombat(DeltaSeconds);
	TickSlide(DeltaSeconds);
	UpdateWeaponAttachment(DeltaSeconds);
	UpdateFollowCamera(DeltaSeconds);
	UpdateLocomotionAnimation();
}

void AEOOperativeCharacter::UpdateFollowCamera(float DeltaSeconds)
{
	LookHoldRemaining = FMath::Max(0.f, LookHoldRemaining - DeltaSeconds);
	MoveInputHoldRemaining = FMath::Max(0.f, MoveInputHoldRemaining - DeltaSeconds);

	AController* OwningController = GetController();
	if (!OwningController || DeltaSeconds <= 0.f || MoveInputHoldRemaining <= 0.f)
	{
		return;
	}

	// Aiming hands the camera to the mouse outright, and a traversal is already
	// driving the capsule along an authored arc - swinging the view during either
	// would be the camera arguing with the player.
	if (bAiming || LookHoldRemaining > 0.f || !HasControl())
	{
		return;
	}

	if (Traversal && Traversal->IsTraversing())
	{
		return;
	}

	const FVector Flat = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
	const float Speed = Flat.Size();
	if (Speed < CameraFollowMinSpeed)
	{
		return;
	}

	const FRotator Control = OwningController->GetControlRotation();
	const float Delta = FRotator::NormalizeAxis(Flat.Rotation().Yaw - Control.Yaw);

	// A dead zone, or the camera hunts around the exact heading forever and the
	// whole view develops a permanent low-level wobble.
	if (FMath::Abs(Delta) < CameraFollowDeadZone)
	{
		return;
	}

	// Faster the quicker the operative is going: a walk should barely drag the
	// camera round, a sprint should put it behind them promptly.
	const float SpeedAlpha = FMath::Clamp(Speed / FMath::Max(SprintSpeed, 1.f), 0.f, 1.f);
	const float MaxStep = CameraFollowRate * SpeedAlpha * DeltaSeconds;

	OwningController->SetControlRotation(FRotator(Control.Pitch,
		Control.Yaw + FMath::Clamp(Delta, -MaxStep, MaxStep), Control.Roll));
}

// ---------------------------------------------------------------------- slide

bool AEOOperativeCharacter::TryStartSlide()
{
	if (bSliding || !HasControl() || (Traversal && Traversal->IsTraversing()))
	{
		return false;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || Movement->IsFalling())
	{
		return false;
	}

	// A slide is a sprint that goes low. Crouching from a standstill is just
	// crouching, and sliding out of it would feel like a glitch.
	const FVector Velocity = Movement->Velocity;
	if (Velocity.Size2D() < SlideEntrySpeed)
	{
		return false;
	}

	bSliding = true;
	SlideElapsed = 0.f;
	SlideDirection = Velocity.GetSafeNormal2D();
	SlideSpeed = FMath::Max(Velocity.Size2D(), SlideImpulse);

	// Hand deceleration to TickSlide. Left alone, the movement component's ground
	// friction and braking would scrub the slide off in under half a second, and
	// the slide would end before it had gone anywhere.
	CachedGroundFriction = Movement->GroundFriction;
	CachedBrakingDeceleration = Movement->BrakingDecelerationWalking;
	Movement->GroundFriction = 0.f;
	Movement->BrakingDecelerationWalking = 0.f;

	Crouch();
	Movement->Velocity = SlideDirection * SlideSpeed;

	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		FEOFeedbackContext Context = FEOFeedbackContext::AtActor(this);
		Context.AttachTo = GetRootComponent();
		Feedback->Play(EOFeedbackEvents::Parkour_Slide, Context);
	}

	return true;
}

void AEOOperativeCharacter::StopSlide()
{
	if (!bSliding)
	{
		return;
	}

	bSliding = false;
	SlideElapsed = 0.f;
	SlideSpeed = 0.f;
	UnCrouch();

	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		Feedback->Play(EOFeedbackEvents::Parkour_SlideEnd, FEOFeedbackContext::AtActor(this));
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
		Movement->MaxWalkSpeedCrouched = SlideImpulse;
		Movement->GroundFriction = CachedGroundFriction;
		Movement->BrakingDecelerationWalking = CachedBrakingDeceleration;
	}
}

void AEOOperativeCharacter::TickSlide(float DeltaSeconds)
{
	if (!bSliding)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		StopSlide();
		return;
	}

	SlideElapsed += DeltaSeconds;

	// Carry the entry direction and bleed off at the slide's own rate, rather
	// than letting the player steer freely: a slide is a commitment, not a
	// crouch-walk.
	SlideSpeed = FMath::Max(SlideSpeed - SlideFriction * DeltaSeconds, 0.f);
	Movement->Velocity = SlideDirection * SlideSpeed;

	// While crouched it is MaxWalkSpeedCrouched that caps movement; clamping
	// MaxWalkSpeed instead would let the default 300 kill the slide instantly.
	Movement->MaxWalkSpeedCrouched = FMath::Max(SlideSpeed, SlideExitSpeed);

	if (SlideSpeed <= SlideExitSpeed || SlideElapsed >= SlideMaxDuration || Movement->IsFalling())
	{
		StopSlide();
	}
}

// ------------------------------------------------------------------ animation

bool AEOOperativeCharacter::UsesDirectAnimationPlayback() const
{
	// An Animation Blueprint owns the pose when one is assigned, and
	// PlayAnimation would silently switch the mesh back to single-node playback
	// and throw the whole graph away. So every direct call is gated on the mesh
	// actually being in single-node mode - which is the greybox fallback for a
	// project whose animation assets have not been prepared yet.
	const USkeletalMeshComponent* MeshComp = GetMesh();
	return MeshComp && MeshComp->GetAnimationMode() == EAnimationMode::AnimationSingleNode;
}

bool AEOOperativeCharacter::PlayActionAnimation(UAnimMontage* Montage, UAnimSequence* Fallback)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return false;
	}

	// With a graph in charge, a one-shot is a montage played into its slot: the
	// legs keep running the locomotion state machine underneath, and the action
	// blends in and out rather than replacing the whole body.
	if (!UsesDirectAnimationPlayback())
	{
		if (!Montage)
		{
			return false;
		}

		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			return AnimInstance->Montage_Play(Montage) > 0.f;
		}

		return false;
	}

	// Greybox fallback: no graph, so drive the clip directly.
	if (!Fallback)
	{
		return false;
	}

	MeshComp->PlayAnimation(Fallback, false);
	CurrentAnim = Fallback;
	return true;
}

void AEOOperativeCharacter::UpdateLocomotionAnimation()
{
	if (!UsesDirectAnimationPlayback())
	{
		// The graph reads velocity and falling state off the movement component
		// itself: looping, blending, starts, stops and landings are its job, and
		// it does them far better than a clip-per-state switch could.
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!MeshComp || !Movement)
	{
		return;
	}

	// Traversals own the mesh for their duration and play their own one-shot.
	if (Traversal && Traversal->IsTraversing())
	{
		CurrentAnim = nullptr;
		return;
	}

	UAnimSequence* Wanted = nullptr;
	bool bLoop = true;

	// Death holds its final pose; the takedown and the shot are one-shots that
	// must be allowed to finish before locomotion takes the mesh back.
	if (IsDead())
	{
		return;
	}

	if (IsPerformingTakedown())
	{
		return;
	}

	if (CurrentAnim == FireAnim && FireAnim && MeshComp->IsPlaying())
	{
		return;
	}

	// Authored speed of whatever gets chosen, so the play rate can be matched to
	// the ground speed below.
	float ClipSpeed = 0.f;

	if (bAiming && !bSliding && !Movement->IsFalling())
	{
		const float AimSpeed = Movement->Velocity.Size2D();

		if (AimSpeed < 10.f)
		{
			Wanted = AimAnim ? AimAnim : IdleAnim;
		}
		else
		{
			// Aiming locks the body to the crosshair and strafes, so the clip is
			// chosen by which way the operative is travelling relative to where
			// they are facing - not by how fast.
			const FVector Facing = GetActorForwardVector().GetSafeNormal2D();
			const FVector Right = GetActorRightVector().GetSafeNormal2D();
			const FVector Travel = Movement->Velocity.GetSafeNormal2D();

			const float Forward = FVector::DotProduct(Facing, Travel);
			const float Lateral = FVector::DotProduct(Right, Travel);

			if (FMath::Abs(Forward) >= FMath::Abs(Lateral))
			{
				Wanted = (Forward >= 0.f) ? AimStrafeForward : AimStrafeBackward;
			}
			else
			{
				Wanted = (Lateral >= 0.f) ? AimStrafeRight : AimStrafeLeft;
			}

			// Fall back to the standing aim rather than popping to an unarmed run
			// if a strafe clip has not been assigned.
			if (!Wanted)
			{
				Wanted = AimAnim ? AimAnim : IdleAnim;
			}
			else
			{
				ClipSpeed = GetClipSpeed(Wanted, StrafeAnimSpeed);
			}
		}
	}
	else if (bSliding)
	{
		Wanted = SlideAnim;
	}
	else if (Movement->IsFalling())
	{
		Wanted = FallAnim;
	}
	else
	{
		const float Speed = Movement->Velocity.Size2D();
		if (Speed < 10.f)
		{
			Wanted = IdleAnim;
		}
		else if (Speed > SprintSpeed * 0.9f)
		{
			Wanted = SprintAnim;
			ClipSpeed = GetClipSpeed(SprintAnim, SprintAnimSpeed);
		}
		else if (Speed > WalkSpeed * 0.6f)
		{
			Wanted = JogAnim;
			ClipSpeed = GetClipSpeed(JogAnim, JogAnimSpeed);
		}
		else
		{
			Wanted = WalkAnim;
			ClipSpeed = GetClipSpeed(WalkAnim, WalkAnimSpeed);
		}
	}

	// Only restart on a genuine change, or the clip resets to frame zero every tick.
	if (Wanted && Wanted != CurrentAnim)
	{
		MeshComp->PlayAnimation(Wanted, bLoop);
		CurrentAnim = Wanted;
	}

	// Match the cycle to the ground speed. Re-applied every frame because speed
	// changes continuously within a band - accelerating out of a standing start
	// otherwise plays a full-speed run cycle over a character barely moving.
	if (Wanted && ClipSpeed > KINDA_SMALL_NUMBER)
	{
		const float Rate = FMath::Clamp(Movement->Velocity.Size2D() / ClipSpeed,
			MinAnimPlayRate, MaxAnimPlayRate);
		MeshComp->SetPlayRate(Rate);
	}
	else if (Wanted)
	{
		MeshComp->SetPlayRate(1.f);
	}
}

TArray<UAnimSequence*> AEOOperativeCharacter::GetLocomotionClips() const
{
	TArray<UAnimSequence*> Clips;
	for (UAnimSequence* Clip : { WalkAnim.Get(), JogAnim.Get(), SprintAnim.Get() })
	{
		if (Clip)
		{
			Clips.Add(Clip);
		}
	}
	return Clips;
}

float AEOOperativeCharacter::GetClipSpeed(UAnimSequence* Clip, float Fallback)
{
	if (!Clip)
	{
		return Fallback;
	}

	if (const float* Cached = MeasuredClipSpeeds.Find(Clip))
	{
		return *Cached;
	}

	// How far the root actually travels over the clip, which is the only honest
	// answer to "what speed was this authored at". The hand-entered numbers this
	// replaced were out by up to a factor of two - the sprint loop was listed at
	// 800 cm/s and covers 415 - which is why the run cycle never matched the
	// ground underneath it.
	float Speed = Fallback;
	const float Length = Clip->GetPlayLength();
	if (Length > KINDA_SMALL_NUMBER)
	{
		const FTransform Delta = Clip->ExtractRootMotionFromRange(0.0, Length, FAnimExtractContext());
		const float Distance = Delta.GetTranslation().Size2D();

		// An in-place clip has nothing to measure; fall back rather than divide a
		// few centimetres of foot jitter into a speed.
		if (Distance > 10.f)
		{
			Speed = Distance / Length;
		}
	}

	MeasuredClipSpeeds.Add(Clip, Speed);
	return Speed;
}

const UEOInputConfig* AEOOperativeCharacter::GetInputConfig() const
{
	const AEOPlayerController* EOController = Cast<AEOPlayerController>(GetController());
	return EOController ? EOController->GetInputConfig() : nullptr;
}

void AEOOperativeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const UEOInputConfig* Config = GetInputConfig();
	if (!Input || !Config)
	{
		UE_LOG(LogExecutiveOps, Error, TEXT("Operative input setup failed: missing EnhancedInputComponent or input config."));
		return;
	}

	Input->BindAction(Config->MoveAction, ETriggerEvent::Triggered, this, &AEOOperativeCharacter::Input_Move);
	Input->BindAction(Config->LookAction, ETriggerEvent::Triggered, this, &AEOOperativeCharacter::Input_Look);
	Input->BindAction(Config->LookStickAction, ETriggerEvent::Triggered, this, &AEOOperativeCharacter::Input_LookStick);
	Input->BindAction(Config->JumpAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_Jump);
	Input->BindAction(Config->JumpAction, ETriggerEvent::Completed, this, &AEOOperativeCharacter::Input_StopJump);
	Input->BindAction(Config->SprintAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_SprintStart);
	Input->BindAction(Config->SprintAction, ETriggerEvent::Completed, this, &AEOOperativeCharacter::Input_SprintStop);
	Input->BindAction(Config->SlideAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_SlideStart);
	Input->BindAction(Config->SlideAction, ETriggerEvent::Completed, this, &AEOOperativeCharacter::Input_SlideStop);
	Input->BindAction(Config->TakedownAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_Takedown);
	Input->BindAction(Config->FireAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_Fire);
	Input->BindAction(Config->AimAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_AimStart);
	Input->BindAction(Config->AimAction, ETriggerEvent::Completed, this, &AEOOperativeCharacter::Input_AimStop);
	Input->BindAction(Config->InteractAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_Interact);
}

void AEOOperativeCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero() || !HasControl())
	{
		return;
	}

	// The traversal drives the capsule directly; letting the movement component
	// also act on input makes the two fight over the same transform. The takedown
	// is likewise a commitment - running out of one slides the operative across
	// the floor in the strike pose.
	if ((Traversal && Traversal->IsTraversing()) || IsPerformingTakedown())
	{
		return;
	}

	// The auto-follow keys off this rather than off velocity. Residual motion is
	// not steering: a teleport, a shove or a slide's run-out should not drag the
	// view round, and scripted code that sets the control rotation deliberately
	// must not have it stolen back a frame later.
	MoveInputHoldRemaining = MoveInputHoldTime;

	// Move relative to where the camera is looking, flattened to the ground plane.
	const FRotator YawOnly(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y), Axis.X);
}

void AEOOperativeCharacter::Input_Look(const FInputActionValue& Value)
{
	// Looking around during the descent is intended; looking while stowed is not.
	if (bStowed)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();

	// AddControllerPitchInput subtracts from pitch, and Mouse2D reports +Y when
	// the mouse moves up, so the raw axis looks down when pushed up. The standard
	// scheme is that negation; the player's inversion preference multiplies it,
	// read per event so that changing it applies immediately.
	AddControllerYawInput(Axis.X * LookSensitivity);
	AddControllerPitchInput(-Axis.Y * LookSensitivity * UEOInputSettings::MouseSign());

	MarkLookInput(Axis);
}

void AEOOperativeCharacter::Input_LookStick(const FInputActionValue& Value)
{
	if (bStowed)
	{
		return;
	}

	// A stick is a rate, not a delta: without delta time the view speed scales
	// with frame rate.
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Scale = StickLookRate * GetWorld()->GetDeltaSeconds();

	AddControllerYawInput(Axis.X * Scale);
	AddControllerPitchInput(-Axis.Y * Scale * UEOInputSettings::StickSign());

	MarkLookInput(Axis);
}

void AEOOperativeCharacter::Input_Jump(const FInputActionValue& Value)
{
	if (!HasControl())
	{
		return;
	}

	// Contextual: vault, mantle or climb whatever is in front, and only jump when
	// there is nothing to traverse. A miss leaves the press buffered, so pressing
	// a stride early still clears the obstacle.
	if (Traversal && Traversal->TryTraverse())
	{
		return;
	}

	Jump();
}

void AEOOperativeCharacter::Input_StopJump(const FInputActionValue& Value)
{
	StopJumping();
}

void AEOOperativeCharacter::Input_SprintStart(const FInputActionValue& Value)
{
	if (!HasControl())
	{
		return;
	}

	bSprinting = true;
	if (!bSliding)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AEOOperativeCharacter::Input_SprintStop(const FInputActionValue& Value)
{
	bSprinting = false;
	if (!bSliding)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void AEOOperativeCharacter::Input_SlideStart(const FInputActionValue& Value)
{
	if (!HasControl() || IsDead())
	{
		return;
	}

	if (!TryStartSlide())
	{
		Crouch();
	}
}

void AEOOperativeCharacter::Input_Takedown(const FInputActionValue& Value)
{
	TryTakedown();
}

void AEOOperativeCharacter::Input_Fire(const FInputActionValue& Value)
{
	FireWeapon();
}

void AEOOperativeCharacter::Input_AimStart(const FInputActionValue& Value)
{
	SetAiming(HasControl());
}

void AEOOperativeCharacter::Input_AimStop(const FInputActionValue& Value)
{
	SetAiming(false);
}

void AEOOperativeCharacter::SetAiming(bool bNewAiming)
{
	if (bAiming == bNewAiming)
	{
		return;
	}

	bAiming = bNewAiming;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Aiming swaps the whole control relationship. Free movement steers the body
	// and lets the camera trail it; aiming pins the body to the camera, so the
	// mouse points the operative and A/D become strafes rather than turns. Both
	// are third-person schemes, but mixing them is what makes aim feel vague.
	bUseControllerRotationYaw = bAiming;
	Movement->bOrientRotationToMovement = !bAiming;

	if (bAiming)
	{
		// Snap the body to where the player is already looking, so the first shot
		// is not fired by someone still rotating into place.
		if (const AController* OwningController = GetController())
		{
			SetActorRotation(FRotator(0.f, OwningController->GetControlRotation().Yaw, 0.f));
		}
	}
}

void AEOOperativeCharacter::MarkLookInput(const FVector2D& Axis)
{
	if (!Axis.IsNearlyZero())
	{
		// Any deliberate look suspends the auto-follow. The player's aim always
		// outranks the camera's opinion about where it should be.
		LookHoldRemaining = LookHoldTime;
	}
}

void AEOOperativeCharacter::Input_Interact(const FInputActionValue& Value)
{
	TryInteract();
}

void AEOOperativeCharacter::Input_SlideStop(const FInputActionValue& Value)
{
	// Releasing crouch ends a slide early; the slide also ends on its own.
	StopSlide();
	UnCrouch();
}

void AEOOperativeCharacter::SetStowed_Implementation(bool bInStowed)
{
	bStowed = bInStowed;

	// Stowing is how an abandoned drop ends, so it has to end the drop. Relying on
	// callers to cancel first worked only because every caller remembered to.
	if (bStowed)
	{
		EndDeploymentDrop();
	}

	// Order matters: Traversal::Finish re-enables capsule collision, so cancelling
	// after the disable below would leave a hidden operative with a live blocking
	// capsule parked wherever its vault stopped.
	if (Traversal)
	{
		Traversal->Cancel();
	}

	if (bStowed)
	{
		// Nothing about the last insertion should carry into the next one.
		StopSlide();
		bSprinting = false;
		bAiming = false;
		Weapon->ResetWeapon();
		TakedownRemaining = 0.f;
		CurrentAnim = nullptr;
		UnCrouch();
	}

	SetActorHiddenInGame(bStowed);
	SetActorEnableCollision(!bStowed);
	SetActorTickEnabled(!bStowed);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		// Gravity would drag a stowed operative through the floor while it waits.
		Movement->SetMovementMode(bStowed ? MOVE_None : MOVE_Walking);
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

void AEOOperativeCharacter::UnPossessed()
{
	Super::UnPossessed();

	// Clearing the mapping context can tear down an in-progress action without a
	// Completed event, which would otherwise leave the operative sprinting forever.
	bSprinting = false;
	bAiming = false;
	StopSlide();
	UnCrouch();

	if (Traversal)
	{
		Traversal->Cancel();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

bool AEOOperativeCharacter::EndDeploymentDrop()
{
	if (!bDeploying)
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(DropTimeoutTimer);
	bDeploying = false;
	return true;
}

void AEOOperativeCharacter::BeginDeploymentDrop(const FTransform& FromSocket, const FVector& LaunchVelocity)
{
	// A traversal mid-flight would otherwise keep interpolating toward an
	// obstacle on the other side of the level.
	if (Traversal)
	{
		Traversal->Cancel();
	}
	StopSlide();

	// Leaving the aim scheme on through the drop would pin the body to the camera
	// for the whole descent, and land the operative strafing.
	SetAiming(false);

	Execute_SetStowed(this, false);

	SetActorLocationAndRotation(FromSocket.GetLocation(), FromSocket.GetRotation(),
		/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	bDeploying = true;

	// Thrown clear rather than dropped: the operative is leaving under power.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Falling);
		Movement->Velocity = LaunchVelocity;
	}

	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		FEOFeedbackContext Context = FEOFeedbackContext::AtActor(this);
		Context.AttachTo = GetRootComponent();
		Feedback->Play(EOFeedbackEvents::Deploy_Launch, Context);
	}

	GetWorldTimerManager().SetTimer(
		DropTimeoutTimer, this, &AEOOperativeCharacter::OnDropTimedOut, DropTimeout, false);
}

void AEOOperativeCharacter::Landed(const FHitResult& Hit)
{
	// Read the impact speed before Super runs: the movement component zeroes
	// vertical velocity as part of landing, and afterwards every landing looks
	// equally gentle.
	const float ImpactSpeed = FMath::Abs(GetVelocity().Z);

	Super::Landed(Hit);

	if (UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		FEOFeedbackContext Context = FEOFeedbackContext::At(Hit.ImpactPoint);
		Context.Rotation = Hit.ImpactNormal.Rotation();

		if (bDeploying)
		{
			// The drop's own landing is a signature beat, not a generic one.
			Feedback->Play(EOFeedbackEvents::Deploy_Land, Context);
		}
		else if (ImpactSpeed > HardLandingSpeed)
		{
			Context.Scale = FMath::Clamp(ImpactSpeed / (HardLandingSpeed * 2.f), 0.6f, 1.5f);
			Feedback->Play(EOFeedbackEvents::Move_LandHard, Context);
		}
		else if (ImpactSpeed > HardLandingSpeed * 0.35f)
		{
			Feedback->Play(EOFeedbackEvents::Move_LandLight, Context);
		}
	}

	if (!EndDeploymentDrop())
	{
		return;
	}

	Execute_OnDeployComplete(this);

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->CompleteDeployment();
	}
}

void AEOOperativeCharacter::CancelDeploymentDrop()
{
	if (!EndDeploymentDrop())
	{
		return;
	}

	if (Traversal)
	{
		Traversal->Cancel();
	}
	StopSlide();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	UE_LOG(LogExecutiveOps, Log, TEXT("Deployment drop cancelled."));
}

void AEOOperativeCharacter::OnDropTimedOut()
{
	if (!EndDeploymentDrop())
	{
		return;
	}

	// Landing never fired - dropped into geometry, or off the edge of the world.
	// Hand control back anyway rather than leaving the player as a spectator.
	UE_LOG(LogExecutiveOps, Warning,
		TEXT("Deployment drop timed out after %.1fs; recovering to the insertion point."), DropTimeout);

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();

	// The drop never landed: stuck in geometry, or falling past the world floor.
	// Unlocking input alone would hand the player a broken situation, so put them
	// on the site's insertion point before giving control back.
	if (const AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr)
	{
		const FTransform Insertion = Site->GetInsertionTransform();
		SetActorLocationAndRotation(Insertion.GetLocation(), Insertion.GetRotation(),
			/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Falling);
	}

	Execute_OnDeployComplete(this);

	if (Mission)
	{
		Mission->CompleteDeployment();
	}
}

void AEOOperativeCharacter::OnDeployFrom_Implementation(AActor* SourceAircraft, const FTransform& DeploySocket)
{
	if (Traversal)
	{
		Traversal->Cancel();
	}

	// Must come out of stow before being placed, or it would land with no collision.
	Execute_SetStowed(this, false);

	SetActorLocationAndRotation(DeploySocket.GetLocation(), DeploySocket.GetRotation(),
		/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogExecutiveOps, Log, TEXT("Operative deployed from %s."), *GetNameSafe(SourceAircraft));
}

void AEOOperativeCharacter::OnDeployComplete_Implementation()
{
	UE_LOG(LogExecutiveOps, Log, TEXT("Operative deployment complete; ground control active."));
}

void AEOOperativeCharacter::OnExtractBegin_Implementation(AActor* TargetAircraft)
{
	UE_LOG(LogExecutiveOps, Log, TEXT("Operative extracting to %s."), *GetNameSafe(TargetAircraft));
}
