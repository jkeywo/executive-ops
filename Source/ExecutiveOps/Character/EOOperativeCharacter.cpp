#include "Character/EOOperativeCharacter.h"

#include "Camera/CameraComponent.h"
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

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
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
}

void AEOOperativeCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero() || !HasControl())
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
	if (HasControl())
	{
		Jump();
	}
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
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AEOOperativeCharacter::Input_SprintStop(const FInputActionValue& Value)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AEOOperativeCharacter::SetStowed_Implementation(bool bInStowed)
{
	bStowed = bInStowed;

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
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

void AEOOperativeCharacter::BeginDeploymentDrop(const FTransform& FromSocket, const FVector& LaunchVelocity)
{
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
