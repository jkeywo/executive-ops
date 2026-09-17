#include "Aircraft/EOAircraftPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "ExecutiveOps.h"
#include "GameFramework/SpringArmComponent.h"
#include "Core/EOPlayerController.h"
#include "Input/EOInputConfig.h"
#include "UI/EONavigationHUD.h"

AEOAircraftPawn::AEOAircraftPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(FVector(400.f, 300.f, 120.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	// APawn defaults bUseControllerRotationYaw to true, which makes the controller
	// overwrite the hull's rotation every frame and silently destroys yaw input.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Visuals bank independently of collision, so the hull can roll into a turn
	// without the collision box rolling with it and catching on geometry.
	HullPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HullPivot"));
	HullPivot->SetupAttachment(RootComponent);

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	HullMesh->SetupAttachment(HullPivot);
	HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ThrusterLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThrusterLeft"));
	ThrusterLeft->SetupAttachment(HullPivot);
	ThrusterLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThrusterLeft->SetRelativeLocation(FVector(-320.f, -260.f, -60.f));

	ThrusterRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThrusterRight"));
	ThrusterRight->SetupAttachment(HullPivot);
	ThrusterRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThrusterRight->SetRelativeLocation(FVector(-320.f, 260.f, -60.f));

	DeploymentSocket = CreateDefaultSubobject<USceneComponent>(TEXT("DeploymentSocket"));
	DeploymentSocket->SetupAttachment(RootComponent);
	// Far enough below the hull that the operative's capsule (96cm half-height)
	// clears the collision box (120cm half-extent) instead of spawning inside it
	// and being shoved sideways by depenetration on its first movement tick.
	DeploymentSocket->SetRelativeLocation(FVector(0.f, 0.f, -350.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CameraDistanceHover;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 320.f);
	// The boom inherits the craft's yaw so the camera sits behind the nose. Control
	// rotation is deliberately not used: the craft yaws with Q/E, not the mouse.
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 6.f;
	CameraBoom->CameraRotationLagSpeed = 8.f;
	CameraBoom->bDoCollisionTest = false;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ChaseCamera->bUsePawnControlRotation = false;
	ChaseCamera->SetFieldOfView(CameraFOVBase);

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(RootComponent);
	EngineAudio->bAutoActivate = false;
}

void AEOAircraftPawn::BeginPlay()
{
	Super::BeginPlay();

	// Audio is entirely optional in M1: the hook exists so M8 only has to assign a
	// sound, but an unset asset must not produce a warning every frame.
	if (EngineLoopSound && EngineAudio)
	{
		EngineAudio->SetSound(EngineLoopSound);
		EngineAudio->Play();
	}
}

const UEOInputConfig* AEOAircraftPawn::GetInputConfig() const
{
	const AEOPlayerController* EOController = Cast<AEOPlayerController>(GetController());
	return EOController ? EOController->GetInputConfig() : nullptr;
}

float AEOAircraftPawn::GetSpeedAlpha() const
{
	return FlightMaxSpeed > KINDA_SMALL_NUMBER
		? FMath::Clamp(Velocity.Size() / FlightMaxSpeed, 0.f, 1.f)
		: 0.f;
}

void AEOAircraftPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Blend towards the requested mode before anything reads the handling values,
	// so a mode change eases in rather than snapping.
	const float BlendRate = DeltaSeconds / FMath::Max(HoverBlendTime, KINDA_SMALL_NUMBER);
	HoverBlend = FMath::Clamp(HoverBlend + (bHoverRequested ? BlendRate : -BlendRate), 0.f, 1.f);

	UpdateFlight(DeltaSeconds);
	UpdateAttitude(DeltaSeconds);
	UpdateLook(DeltaSeconds);
	UpdateCamera(DeltaSeconds);
	UpdateFeedback(DeltaSeconds);

	bLookActiveThisFrame = false;
}

bool AEOAircraftPawn::UpdateScriptedArrival(float DeltaSeconds)
{
	if (!bScriptedArrival)
	{
		return false;
	}

	const FVector ToTarget = ScriptedDestination - GetActorLocation();
	const float Distance = ToTarget.Size();

	// Turn the nose toward where it is going, so the arrival reads as the craft
	// flying in rather than sliding sideways on rails.
	const FVector Flat(ToTarget.X, ToTarget.Y, 0.f);
	if (Flat.SizeSquared() > FMath::Square(200.f))
	{
		const FRotator Desired(0.f, Flat.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, 3.f));
	}

	// Speed proportional to remaining distance: it slows into the hover instead
	// of arriving at full speed and needing to be caught.
	const float DesiredSpeed = FMath::Min(FlightMaxSpeed, Distance * ScriptedApproachGain);
	const FVector Target = Distance > KINDA_SMALL_NUMBER
		? ToTarget / Distance * DesiredSpeed
		: FVector::ZeroVector;

	Velocity = FMath::VInterpConstantTo(Velocity, Target, DeltaSeconds, FlightAcceleration);
	ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.6f, DeltaSeconds, 4.f);

	if (!Velocity.IsNearlyZero())
	{
		FHitResult Hit;
		AddActorWorldOffset(Velocity * DeltaSeconds, /*bSweep=*/true, &Hit);

		if (Hit.bBlockingHit)
		{
			if (Hit.bStartPenetrating)
			{
				// Same escape the piloted path uses: projecting velocity onto the
				// surface would delete the component that gets the craft clear.
				AddActorWorldOffset(Hit.Normal * (Hit.PenetrationDepth + 1.f), /*bSweep=*/false);
			}
			else
			{
				// Clipped a building on the way in. Slide rather than stopping
				// dead, damped per unit time so the penalty does not depend on
				// frame rate - a per-frame halving stalls the craft completely
				// at high refresh rates.
				Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal)
					* FMath::Pow(SlideRetentionPerSecond, DeltaSeconds);
			}
		}
	}

	// Arrived means settled, not merely passing through: a craft still doing
	// 40 m/s through the radius has not stopped to collect anyone.
	bScriptedArrived = (Distance <= ScriptedArriveRadius) && (Velocity.Size() < 400.f);
	return true;
}

void AEOAircraftPawn::UpdateFlight(float DeltaSeconds)
{
	if (UpdateScriptedArrival(DeltaSeconds))
	{
		return;
	}

	if (bDeploymentHold)
	{
		// Genuinely parked: velocity was zeroed the instant hold was set, so the
		// craft does not creep for a few frames after the player commits, and
		// there is no unchecked sweep here to leave it penetrating.
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.15f, DeltaSeconds, 3.f);
		return;
	}

	const float MaxSpeed = FMath::Lerp(FlightMaxSpeed, HoverMaxSpeed, HoverBlend);
	const float Accel = FMath::Lerp(FlightAcceleration, HoverAcceleration, HoverBlend);
	const float Braking = FMath::Lerp(FlightBraking, HoverBraking, HoverBlend);
	const float YawRate = FMath::Lerp(FlightYawRate, HoverYawRate, HoverBlend);

	if (!FMath::IsNearlyZero(YawInput))
	{
		const float YawDelta = YawInput * YawRate * DeltaSeconds;
		AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));

		// Carry momentum around with the nose. Without this the craft keeps its old
		// world heading through a turn and slides sideways like it is on ice.
		Velocity = FRotator(0.f, YawDelta * VelocityTurnFactor, 0.f).RotateVector(Velocity);
	}

	// Build the desired direction in the craft's own frame. Vertical stays world-up
	// so climbing is always straight up regardless of how the hull is banked.
	FVector Desired = FVector::ZeroVector;
	Desired += GetActorForwardVector() * MoveInput.X;
	Desired += GetActorRightVector() * MoveInput.Y;
	Desired += FVector::UpVector * MoveInput.Z;

	if (Desired.IsNearlyZero() && bStationKeepEnabled)
	{
		// Assist: with no pilot input, ease onto the deployment point instead of
		// simply stopping wherever the craft happened to drift to.
		const FVector ToTarget = StationKeepTarget - GetActorLocation();
		const FVector Assist = (ToTarget * StationKeepGain).GetClampedToMaxSize(StationKeepMaxSpeed);
		Velocity = FMath::VInterpConstantTo(Velocity, Assist, DeltaSeconds, Accel);
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.25f, DeltaSeconds, 3.f);
	}
	else if (Desired.IsNearlyZero())
	{
		Velocity = FMath::VInterpConstantTo(Velocity, FVector::ZeroVector, DeltaSeconds, Braking);
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.f, DeltaSeconds, 3.f);
	}
	else
	{
		const FVector Target = Desired.GetClampedToMaxSize(1.f) * MaxSpeed;
		Velocity = FMath::VInterpConstantTo(Velocity, Target, DeltaSeconds, Accel);
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 1.f, DeltaSeconds, 6.f);
	}

	// Bleed down to the current maximum at the braking rate rather than clamping
	// hard. A hard clamp would make HoverBlendTime dictate the whole deceleration
	// when entering hover at speed, bypassing the braking model entirely.
	const float Speed = Velocity.Size();
	if (Speed > MaxSpeed)
	{
		const float Shed = FMath::Max(MaxSpeed, Speed - Braking * DeltaSeconds);
		Velocity = Velocity.GetSafeNormal() * Shed;
	}

	if (Velocity.IsNearlyZero())
	{
		return;
	}

	FHitResult Hit;
	AddActorWorldOffset(Velocity * DeltaSeconds, /*bSweep=*/true, &Hit);

	if (!Hit.bBlockingHit)
	{
		return;
	}

	if (Hit.bStartPenetrating)
	{
		// A sweep that begins penetrating reports zero distance, and projecting
		// velocity onto the surface would delete the very component needed to get
		// out. Push clear along the escape normal instead.
		const FVector Escape = Hit.Normal * (Hit.PenetrationDepth + 1.f);
		AddActorWorldOffset(Escape, /*bSweep=*/false);
		return;
	}

	// Slide along the surface rather than stopping dead: clipping a building at
	// speed should cost momentum, not end the flight. Damped per unit time so the
	// penalty does not depend on frame rate.
	Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal)
		* FMath::Pow(SlideRetentionPerSecond, DeltaSeconds);
}

void AEOAircraftPawn::UpdateAttitude(float DeltaSeconds)
{
	if (!HullPivot)
	{
		return;
	}

	// Bank into strafe and into the turn; pitch the nose down under forward thrust.
	// Both ease off in hover, where the craft should sit flat and stable.
	const float Stability = 1.f - HoverBlend * 0.75f;
	const float TargetRoll = FMath::Clamp(MoveInput.Y + YawInput, -1.f, 1.f) * MaxBankAngle * Stability;
	const float TargetPitch = -MoveInput.X * MaxPitchAngle * Stability;

	const FRotator Current = HullPivot->GetRelativeRotation();
	const FRotator Target(TargetPitch, 0.f, TargetRoll);
	HullPivot->SetRelativeRotation(
		FMath::RInterpTo(Current, Target, DeltaSeconds, AttitudeInterpSpeed));
}

void AEOAircraftPawn::UpdateLook(float DeltaSeconds)
{
	if (!CameraBoom)
	{
		return;
	}

	// Let go of the mouse and the camera swings back behind the craft, so the
	// default view is always the one you fly with.
	if (!bLookActiveThisFrame)
	{
		LookYawOffset = FMath::FInterpTo(LookYawOffset, 0.f, DeltaSeconds, LookRecenterSpeed);
		LookPitchOffset = FMath::FInterpTo(LookPitchOffset, 0.f, DeltaSeconds, LookRecenterSpeed);
	}

	CameraBoom->SetRelativeRotation(FRotator(LookPitchOffset, LookYawOffset, 0.f));
}

void AEOAircraftPawn::UpdateCamera(float DeltaSeconds)
{
	if (!CameraBoom || !ChaseCamera)
	{
		return;
	}

	// Pull back and widen with speed. This is most of what makes fast flight read
	// as fast, so it is driven by actual velocity rather than input.
	const float SpeedAlpha = GetSpeedAlpha();

	const float TargetArm = FMath::Lerp(CameraDistanceHover, CameraDistanceFast, SpeedAlpha);
	CameraBoom->TargetArmLength =
		FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaSeconds, 3.f);

	const float TargetFOV = FMath::Lerp(CameraFOVBase, CameraFOVFast, SpeedAlpha);
	ChaseCamera->SetFieldOfView(
		FMath::FInterpTo(ChaseCamera->FieldOfView, TargetFOV, DeltaSeconds, 3.f));
}

void AEOAircraftPawn::UpdateFeedback(float DeltaSeconds)
{
	// Thrusters swell with engine load. Crude, but it is a readable greybox signal
	// for "the engines are working" without needing a particle asset.
	const float Scale = FMath::Lerp(0.5f, 1.4f, ThrustAlpha);
	const FVector ThrusterScale(Scale, Scale, Scale);
	if (ThrusterLeft)
	{
		ThrusterLeft->SetRelativeScale3D(ThrusterScale);
	}
	if (ThrusterRight)
	{
		ThrusterRight->SetRelativeScale3D(ThrusterScale);
	}

	if (EngineAudio && EngineAudio->IsPlaying())
	{
		EngineAudio->SetPitchMultiplier(
			FMath::Lerp(EnginePitchIdle, EnginePitchMax, FMath::Max(ThrustAlpha, GetSpeedAlpha())));
	}
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
	// Completed zeroes the held demand; without it the throttle would stick on release.
	Input->BindAction(Config->FlightMoveAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_MoveReleased);
	Input->BindAction(Config->FlightVerticalAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_VerticalReleased);
	Input->BindAction(Config->FlightYawAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_YawReleased);

	Input->BindAction(Config->LookAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Look);
	Input->BindAction(Config->HoverAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_HoverStart);
	Input->BindAction(Config->HoverAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_HoverStop);
	Input->BindAction(Config->DeployAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_Deploy);
	Input->BindAction(Config->ToggleMapAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_ToggleMap);
}

void AEOAircraftPawn::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	MoveInput.X = Axis.Y;
	MoveInput.Y = Axis.X;
}

void AEOAircraftPawn::Input_MoveReleased(const FInputActionValue& Value)
{
	MoveInput.X = 0.f;
	MoveInput.Y = 0.f;
}

void AEOAircraftPawn::Input_Vertical(const FInputActionValue& Value)
{
	MoveInput.Z = Value.Get<float>();
}

void AEOAircraftPawn::Input_VerticalReleased(const FInputActionValue& Value)
{
	MoveInput.Z = 0.f;
}

void AEOAircraftPawn::Input_Yaw(const FInputActionValue& Value)
{
	YawInput = Value.Get<float>();
}

void AEOAircraftPawn::Input_YawReleased(const FInputActionValue& Value)
{
	YawInput = 0.f;
}

void AEOAircraftPawn::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}

	// Free-look is a bounded offset on the boom rather than controller rotation,
	// because the controller must not be allowed to rotate the hull.
	LookYawOffset = FMath::Clamp(LookYawOffset + Axis.X * LookSensitivity, -MaxLookYaw, MaxLookYaw);
	LookPitchOffset = FMath::Clamp(LookPitchOffset + Axis.Y * LookSensitivity, -MaxLookPitch, MaxLookPitch);
	bLookActiveThisFrame = true;
}

void AEOAircraftPawn::Input_HoverStart(const FInputActionValue& Value)
{
	Execute_SetHoverEnabled(this, true);
}

void AEOAircraftPawn::Input_HoverStop(const FInputActionValue& Value)
{
	Execute_SetHoverEnabled(this, false);
}

void AEOAircraftPawn::Input_ToggleMap(const FInputActionValue& Value)
{
	if (const AEOPlayerController* EOController = Cast<AEOPlayerController>(GetController()))
	{
		if (AEONavigationHUD* HUD = Cast<AEONavigationHUD>(EOController->GetHUD()))
		{
			HUD->ToggleMap();
		}
	}
}

void AEOAircraftPawn::Input_Deploy(const FInputActionValue& Value)
{
	if (AEOPlayerController* EOController = Cast<AEOPlayerController>(GetController()))
	{
		EOController->RequestDeployment();
	}
}

void AEOAircraftPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ClearControlDemand();
}

void AEOAircraftPawn::UnPossessed()
{
	Super::UnPossessed();

	// Removing the mapping context can tear an in-progress action down without a
	// Completed event, which would otherwise leave hover latched on forever.
	ClearControlDemand();
}

void AEOAircraftPawn::ClearControlDemand()
{
	MoveInput = FVector::ZeroVector;
	YawInput = 0.f;
	bHoverRequested = false;
}

void AEOAircraftPawn::SetFlightInput_Implementation(const FVector& InMoveInput)
{
	MoveInput = InMoveInput;
}

void AEOAircraftPawn::SetYawInput_Implementation(float InYawInput)
{
	YawInput = InYawInput;
}

void AEOAircraftPawn::SetHoverEnabled_Implementation(bool bEnabled)
{
	bHoverRequested = bEnabled;
}

bool AEOAircraftPawn::IsHovering_Implementation() const
{
	return bHoverRequested;
}

float AEOAircraftPawn::GetCurrentSpeed_Implementation() const
{
	return Velocity.Size();
}

FTransform AEOAircraftPawn::GetDeploymentSocketTransform_Implementation() const
{
	return DeploymentSocket->GetComponentTransform();
}

bool AEOAircraftPawn::IsReadyForDeployment_Implementation() const
{
	// M1 proxy for "stabilised over the site": committed to hover and nearly stopped.
	// M3 replaces this with the hover volume test.
	return bHoverRequested && Velocity.Size() < 200.f;
}

void AEOAircraftPawn::SetStationKeepTarget_Implementation(const FVector& WorldLocation, bool bEnabled)
{
	StationKeepTarget = WorldLocation;
	bStationKeepEnabled = bEnabled;
}

void AEOAircraftPawn::SetDeploymentHold_Implementation(bool bHeld)
{
	bDeploymentHold = bHeld;
	if (bHeld)
	{
		ClearControlDemand();
		Velocity = FVector::ZeroVector;
	}
}

void AEOAircraftPawn::SetScriptedDestination_Implementation(const FVector& WorldLocation, bool bEnabled)
{
	// Only a real change invalidates the arrival. The caller refreshes this every
	// frame to track a moving pickup point, and clearing the flag unconditionally
	// would mean the craft could never be observed to have got there.
	const bool bMoved = !ScriptedDestination.Equals(WorldLocation, 50.f);
	if (bMoved || bEnabled != bScriptedArrival)
	{
		bScriptedArrived = false;
	}

	ScriptedDestination = WorldLocation;
	bScriptedArrival = bEnabled;

	if (bEnabled)
	{
		// The craft is flying itself; any held player demand would fight it the
		// moment control came back.
		ClearControlDemand();
		bDeploymentHold = false;
	}
}

bool AEOAircraftPawn::HasReachedScriptedDestination_Implementation() const
{
	return bScriptedArrived;
}

void AEOAircraftPawn::ResetFlightState_Implementation()
{
	MoveInput = FVector::ZeroVector;
	YawInput = 0.f;
	Velocity = FVector::ZeroVector;
	bHoverRequested = false;
	HoverBlend = 0.f;
	ThrustAlpha = 0.f;
	bStationKeepEnabled = false;
	bDeploymentHold = false;
	bScriptedArrival = false;
	bScriptedArrived = false;

	if (HullPivot)
	{
		HullPivot->SetRelativeRotation(FRotator::ZeroRotator);
	}
}
