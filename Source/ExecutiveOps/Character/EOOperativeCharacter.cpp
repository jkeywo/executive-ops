#include "Character/EOOperativeCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "ExecutiveOps.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Core/EOPlayerController.h"
#include "Input/EOInputConfig.h"

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
	Input->BindAction(Config->JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(Config->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(Config->SprintAction, ETriggerEvent::Started, this, &AEOOperativeCharacter::Input_SprintStart);
	Input->BindAction(Config->SprintAction, ETriggerEvent::Completed, this, &AEOOperativeCharacter::Input_SprintStop);
}

void AEOOperativeCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
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
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X * LookSensitivity);
	AddControllerPitchInput(Axis.Y * LookSensitivity);
}

void AEOOperativeCharacter::Input_SprintStart(const FInputActionValue& Value)
{
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
	}
}

void AEOOperativeCharacter::OnDeployFrom_Implementation(AActor* SourceAircraft, const FTransform& DeploySocket)
{
	// Must come out of stow before being placed, or it would land with no collision.
	Execute_SetStowed(this, false);

	// M0: teleport to the socket. M3 replaces this with the launch/drop sequence.
	SetActorLocationAndRotation(DeploySocket.GetLocation(), DeploySocket.GetRotation());
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
