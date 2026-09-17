#include "Character/EOOperativeCharacter.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Character/EOTraversalComponent.h"
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
}

void AEOOperativeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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

	if (bSliding)
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
}

void AEOOperativeCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero() || !HasControl())
	{
		return;
	}

	// The traversal drives the capsule directly; letting the movement component
	// also act on input makes the two fight over the same transform.
	if (Traversal && Traversal->IsTraversing())
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
	if (!HasControl())
	{
		return;
	}

	if (!TryStartSlide())
	{
		Crouch();
	}
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
