#include "Aircraft/EOAircraftPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "ExecutiveOps.h"
#include "GameFramework/SpringArmComponent.h"
#include "Core/EOPlayerController.h"
#include "Input/EOInputConfig.h"

AEOAircraftPawn::AEOAircraftPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(FVector(400.f, 300.f, 120.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	HullMesh->SetupAttachment(RootComponent);
	HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DeploymentSocket = CreateDefaultSubobject<USceneComponent>(TEXT("DeploymentSocket"));
	DeploymentSocket->SetupAttachment(RootComponent);
	DeploymentSocket->SetRelativeLocation(FVector(0.f, 0.f, -150.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1400.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 350.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 5.f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ChaseCamera->bUsePawnControlRotation = false;
}

const UEOInputConfig* AEOAircraftPawn::GetInputConfig() const
{
	const AEOPlayerController* EOController = Cast<AEOPlayerController>(GetController());
	return EOController ? EOController->GetInputConfig() : nullptr;
}

void AEOAircraftPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Yaw first, so movement uses the rotation the player just asked for.
	if (!FMath::IsNearlyZero(PendingYawInput))
	{
		AddActorWorldRotation(FRotator(0.f, PendingYawInput * YawRate * DeltaSeconds, 0.f));
	}

	const float SpeedLimit = bHoverEnabled ? HoverMaxSpeed : MaxSpeed;

	FVector DesiredDirection = FVector::ZeroVector;
	DesiredDirection += GetActorForwardVector() * PendingMoveInput.X;
	DesiredDirection += GetActorRightVector() * PendingMoveInput.Y;
	DesiredDirection += FVector::UpVector * PendingMoveInput.Z;

	if (DesiredDirection.IsNearlyZero())
	{
		// No input: bleed off speed rather than drifting forever.
		Velocity = FMath::VInterpConstantTo(Velocity, FVector::ZeroVector, DeltaSeconds, BrakingDeceleration);
	}
	else
	{
		const FVector TargetVelocity = DesiredDirection.GetClampedToMaxSize(1.f) * SpeedLimit;
		Velocity = FMath::VInterpConstantTo(Velocity, TargetVelocity, DeltaSeconds, Acceleration);
	}

	Velocity = Velocity.GetClampedToMaxSize(SpeedLimit);

	if (!Velocity.IsNearlyZero())
	{
		FHitResult Hit;
		AddActorWorldOffset(Velocity * DeltaSeconds, /*bSweep=*/true, &Hit);
		if (Hit.bBlockingHit)
		{
			Velocity = FVector::ZeroVector;
		}
	}

	// Input is consumed each frame; Enhanced Input re-supplies it while held.
	PendingMoveInput = FVector::ZeroVector;
	PendingYawInput = 0.f;
}

void AEOAircraftPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const UEOInputConfig* Config = GetInputConfig();
	if (!Input || !Config)
	{
		UE_LOG(LogExecutiveOps, Error, TEXT("Aircraft input setup failed: missing EnhancedInputComponent or input config."));
		return;
	}

	Input->BindAction(Config->FlightMoveAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Move);
	Input->BindAction(Config->FlightVerticalAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Vertical);
	Input->BindAction(Config->FlightYawAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Yaw);
	Input->BindAction(Config->LookAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Look);
	Input->BindAction(Config->HoverAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_HoverStart);
	Input->BindAction(Config->HoverAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_HoverStop);
	Input->BindAction(Config->DeployAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_Deploy);
}

void AEOAircraftPawn::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	PendingMoveInput.X = Axis.Y;
	PendingMoveInput.Y = Axis.X;
}

void AEOAircraftPawn::Input_Vertical(const FInputActionValue& Value)
{
	PendingMoveInput.Z = Value.Get<float>();
}

void AEOAircraftPawn::Input_Yaw(const FInputActionValue& Value)
{
	PendingYawInput = Value.Get<float>();
}

void AEOAircraftPawn::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X * LookSensitivity);
	AddControllerPitchInput(Axis.Y * LookSensitivity);
}

void AEOAircraftPawn::Input_HoverStart(const FInputActionValue& Value)
{
	Execute_SetHoverEnabled(this, true);
}

void AEOAircraftPawn::Input_HoverStop(const FInputActionValue& Value)
{
	Execute_SetHoverEnabled(this, false);
}

void AEOAircraftPawn::Input_Deploy(const FInputActionValue& Value)
{
	if (AEOPlayerController* EOController = Cast<AEOPlayerController>(GetController()))
	{
		EOController->RequestDeployment();
	}
}

void AEOAircraftPawn::SetFlightInput_Implementation(const FVector& MoveInput)
{
	PendingMoveInput = MoveInput;
}

void AEOAircraftPawn::SetYawInput_Implementation(float YawInput)
{
	PendingYawInput = YawInput;
}

void AEOAircraftPawn::SetHoverEnabled_Implementation(bool bEnabled)
{
	bHoverEnabled = bEnabled;
}

bool AEOAircraftPawn::IsHovering_Implementation() const
{
	return bHoverEnabled;
}

float AEOAircraftPawn::GetCurrentSpeed_Implementation() const
{
	return Velocity.Size();
}

FTransform AEOAircraftPawn::GetDeploymentSocketTransform_Implementation() const
{
	return DeploymentSocket->GetComponentTransform();
}

void AEOAircraftPawn::ResetFlightState_Implementation()
{
	PendingMoveInput = FVector::ZeroVector;
	PendingYawInput = 0.f;
	Velocity = FVector::ZeroVector;
	bHoverEnabled = false;
}

bool AEOAircraftPawn::IsReadyForDeployment_Implementation() const
{
	// M0 proxy for "stabilised over the site". M3 replaces this with the hover volume test.
	return bHoverEnabled && Velocity.Size() < 200.f;
}
