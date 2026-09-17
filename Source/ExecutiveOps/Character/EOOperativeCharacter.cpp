#include "Character/EOOperativeCharacter.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Character/EOTraversalComponent.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Mission/EOInteractableInterface.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "ExecutiveOps.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Core/EOPlayerController.h"
#include "Input/EOInputConfig.h"
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
	Movement->RotationRate = FRotator(0.f, 640.f, 0.f);
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

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Traversal = CreateDefaultSubobject<UEOTraversalComponent>(TEXT("Traversal"));
	Health = CreateDefaultSubobject<UEOHealthComponent>(TEXT("Health"));

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
	}
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

	Victim->Takedown(this);

	TakedownRemaining = TakedownDuration;
	if (TakedownAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->PlayAnimation(TakedownAnim, false);
			CurrentAnim = TakedownAnim;
		}
	}

	UE_LOG(LogExecutiveOps, Log, TEXT("Takedown on %s."), *Victim->GetName());
	return true;
}

bool AEOOperativeCharacter::FireWeapon()
{
	if (!HasControl() || IsDead() || IsPerformingTakedown() || FireCooldown > 0.f)
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

	FireCooldown = WeaponInterval;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOOperativeShot), false, this);

	// Two stages: the camera decides WHAT is being aimed at, the body decides
	// whether the shot can actually get there. Firing straight from the camera
	// lets the player hit things from behind cover their character is fully
	// hidden by, and clip corners their crosshair is nowhere near.
	const FVector CameraStart = FollowCamera->GetComponentLocation();
	const FVector CameraAim = FollowCamera->GetForwardVector();

	FHitResult CameraHit;
	// ECC_Pawn throughout: the default Pawn profile ignores Visibility, so a
	// Visibility trace passes clean through anything worth shooting.
	const FVector AimPoint = World->LineTraceSingleByChannel(CameraHit, CameraStart,
		CameraStart + CameraAim * WeaponRange, ECC_Pawn, Params)
		? CameraHit.ImpactPoint
		: CameraStart + CameraAim * WeaponRange;

	const FVector Muzzle = GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector Direction = FMath::VRandCone(
		(AimPoint - Muzzle).GetSafeNormal(),
		FMath::DegreesToRadians(bAiming ? AimSpread : HipSpread));

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Muzzle, Muzzle + Direction * WeaponRange,
		ECC_Pawn, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (UEOHealthComponent* HitHealth = HitActor->FindComponentByClass<UEOHealthComponent>())
			{
				HitHealth->ApplyDamage(WeaponDamage, this);
			}
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

	if (DeathAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->PlayAnimation(DeathAnim, false);
			CurrentAnim = DeathAnim;
		}
	}

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
	bAiming = false;
	FireCooldown = 0.f;
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
	FireCooldown = FMath::Max(FireCooldown - DeltaSeconds, 0.f);
	TakedownRemaining = FMath::Max(TakedownRemaining - DeltaSeconds, 0.f);
}

void AEOOperativeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickCombat(DeltaSeconds);
	TickSlide(DeltaSeconds);
	UpdateLocomotionAnimation();
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

void AEOOperativeCharacter::UpdateLocomotionAnimation()
{
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

	if (bAiming && !bSliding && !Movement->IsFalling())
	{
		Wanted = AimAnim ? AimAnim : IdleAnim;
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
		}
		else if (Speed > WalkSpeed * 0.6f)
		{
			Wanted = JogAnim;
		}
		else
		{
			Wanted = WalkAnim;
		}
	}

	// Only restart on a genuine change, or the clip resets to frame zero every tick.
	if (Wanted && Wanted != CurrentAnim)
	{
		MeshComp->PlayAnimation(Wanted, bLoop);
		CurrentAnim = Wanted;
	}
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
	AddControllerYawInput(Axis.X * LookSensitivity);
	AddControllerPitchInput(Axis.Y * LookSensitivity);
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
	bAiming = HasControl();
}

void AEOOperativeCharacter::Input_AimStop(const FInputActionValue& Value)
{
	bAiming = false;
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
		FireCooldown = 0.f;
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

void AEOOperativeCharacter::BeginDeploymentDrop(const FTransform& FromSocket, const FVector& LaunchVelocity)
{
	// A traversal mid-flight would otherwise keep interpolating toward an
	// obstacle on the other side of the level.
	if (Traversal)
	{
		Traversal->Cancel();
	}
	StopSlide();

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

	GetWorldTimerManager().SetTimer(
		DropTimeoutTimer, this, &AEOOperativeCharacter::OnDropTimedOut, DropTimeout, false);
}

void AEOOperativeCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!bDeploying)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DropTimeoutTimer);
	bDeploying = false;

	Execute_OnDeployComplete(this);

	if (UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		Mission->CompleteDeployment();
	}
}

void AEOOperativeCharacter::CancelDeploymentDrop()
{
	if (!bDeploying)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DropTimeoutTimer);
	bDeploying = false;

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
	if (!bDeploying)
	{
		return;
	}

	// Landing never fired - dropped into geometry, or off the edge of the world.
	// Hand control back anyway rather than leaving the player as a spectator.
	UE_LOG(LogExecutiveOps, Warning,
		TEXT("Deployment drop timed out after %.1fs; recovering to the insertion point."), DropTimeout);

	bDeploying = false;

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
