#include "Aircraft/EOAircraftPawn.h"

#include "Aircraft/EOAircraftMovementComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "ExecutiveOps.h"
#include "Feedback/EOFeedbackEvents.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "GameFramework/SpringArmComponent.h"
#include "Core/EOPlayerController.h"
#include "Input/EOInputConfig.h"
#include "Input/EOInputSettings.h"
#include "UI/EODebugHUD.h"
#include "UI/EOHudPanelComponent.h"
#include "UI/EOHudScreenComponent.h"
#include "UI/EONavigationHUD.h"

AEOAircraftPawn::AEOAircraftPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(FVector(400.f, 300.f, 120.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	// The navmesh is generated at runtime and rebuilt around whatever moves
	// through it. A 8m box flying over the arena would have every tile under
	// its path regenerating, and the guard's path requests failing while they
	// did. Characters opt their capsules out of this by default; a vehicle has
	// to say so.
	CollisionBox->SetCanEverAffectNavigation(false);
	RootComponent = CollisionBox;

	// The craft used to integrate its own position in Tick, twice - once for the
	// piloted path and once for the scripted arrival - with no movement component
	// at all, so GetMovementComponent() returned null and nothing in the engine
	// could see it move.
	Movement = CreateDefaultSubobject<UEOAircraftMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = CollisionBox;

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
	// rotation is deliberately not used: in hover the craft yaws with A/D and the
	// mouse only looks around it.
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

	// The cockpit rides on HullPivot rather than the root, so banking rolls the
	// pilot's view with the craft. That roll is most of why first person reads as
	// flying rather than as a camera being dragged through the air.
	CockpitPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CockpitPivot"));
	CockpitPivot->SetupAttachment(HullPivot);

	CockpitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CockpitMesh"));
	CockpitMesh->SetupAttachment(CockpitPivot);
	CockpitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Never in the chase view: from outside, an interior shell is a box of
	// backfaces sitting in the middle of the hull.
	CockpitMesh->SetVisibility(true);

	// Sits at the seat, not on the camera, so the glass stays with the airframe.
	HudScreen = CreateDefaultSubobject<UEOHudScreenComponent>(TEXT("HudScreen"));
	HudScreen->SetupAttachment(CockpitPivot);

	HudPanel = CreateDefaultSubobject<UEOHudPanelComponent>(TEXT("HudPanel"));
	HudPanel->SetupAttachment(CockpitPivot);

	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(CockpitPivot);
	CockpitCamera->bUsePawnControlRotation = false;
	CockpitCamera->SetFieldOfView(CockpitFOV);

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(RootComponent);
	EngineAudio->bAutoActivate = false;
}

void AEOAircraftPawn::BeginPlay()
{
	Super::BeginPlay();

	// The move happens inside the movement component, so the impact comes back out
	// as an event rather than the component knowing what feedback is.
	if (Movement)
	{
		Movement->OnImpact.AddUObject(this, &AEOAircraftPawn::ReportImpact);
	}

	// Audio is entirely optional in M1: the hook exists so M8 only has to assign a
	// sound, but an unset asset must not produce a warning every frame.
	if (EngineLoopSound && EngineAudio)
	{
		EngineAudio->SetSound(EngineLoopSound);
		EngineAudio->Play();
	}

	// Applied rather than assumed: the components are constructed with both
	// cameras active, and whichever one wins would otherwise be arbitrary.
	ApplyViewMode();
}

void AEOAircraftPawn::SetFirstPerson(bool bNewFirstPerson)
{
	if (bFirstPerson == bNewFirstPerson)
	{
		return;
	}

	bFirstPerson = bNewFirstPerson;
	ApplyViewMode();
}

bool AEOAircraftPawn::IsWidgetPanelLive() const
{
	return HudPanel && HudPanel->IsLive();
}

bool AEOAircraftPawn::IsCockpitPanelShowing() const
{
	const USceneComponent* Showing = IsWidgetPanelLive()
		? static_cast<const USceneComponent*>(HudPanel)
		: static_cast<const USceneComponent*>(HudScreen);
	return Showing && Showing->IsVisible();
}

void AEOAircraftPawn::ApplyViewMode()
{
	// The panel only exists in the cockpit; in chase view the HUD goes back to
	// being drawn flat over the screen, because there is no glass to put it on.
	// Two panels exist while the interface is being moved onto widgets, and one
	// draws. The widget panel takes over the moment it has a widget; until then
	// the old one keeps the Canvas readouts on the glass.
	const bool bWidgetPanel = HudPanel && HudPanel->IsLive();

	// Sit the arc's centre exactly on the eye, so the curve is equidistant all
	// the way across and the readouts do not stretch toward the edges. Read off
	// the camera rather than hardcoded, because where the seat is depends on
	// which cockpit model has been imported - at the pivot origin the panel sat
	// most of a metre below the pilot's eyeline, behind the dashboard.
	//
	// Both stay parented to CockpitPivot, not the camera: that is what makes
	// looking around pan the view across a fixed display.
	const FVector Eye = CockpitCamera ? CockpitCamera->GetRelativeLocation() : FVector::ZeroVector;

	if (HudPanel)
	{
		HudPanel->SetRelativeLocation(Eye);
		HudPanel->SetVisibility(bFirstPerson && bWidgetPanel);
	}

	if (HudScreen)
	{
		HudScreen->SetRelativeLocation(Eye);
		HudScreen->SetVisibility(bFirstPerson && !bWidgetPanel);
	}

	// Two casts, both of which can fail: an unpossessed or AI-flown aircraft has
	// no player controller, and chaining through one crashed the flight suite the
	// moment the parked VTOL applied its view mode. The Canvas chain is only
	// redirected into the old panel while that is the one showing.
	if (APlayerController* OwningController = Cast<APlayerController>(GetController()))
	{
		if (AEODebugHUD* Hud = Cast<AEODebugHUD>(OwningController->GetHUD()))
		{
			Hud->SetProjectionScreen((bFirstPerson && !bWidgetPanel) ? HudScreen : nullptr);
		}
	}

	if (CockpitCamera)
	{
		CockpitCamera->SetActive(bFirstPerson);
	}
	if (ChaseCamera)
	{
		ChaseCamera->SetActive(!bFirstPerson);
	}

	// The hull is a solid exterior shell. Sitting inside it, the nose fills the
	// view from behind, so it is hidden to the pilot and restored for the chase.
	//
	// Conditional on someone actually being in the cockpit, not merely on the
	// mode: once the player deploys, the aircraft is scenery to them, and an
	// unoccupied craft flying around as a floating cockpit is a bug the player
	// sees from the ground.
	if (HullMesh)
	{
		const bool bOccupied = GetController() != nullptr;
		HullMesh->SetVisibility(!(bFirstPerson && bHideHullInCockpit && bOccupied));
	}

	// Same reasoning for the interior: from outside it is a box of backfaces in
	// the middle of the hull.
	if (CockpitMesh)
	{
		CockpitMesh->SetVisibility(bFirstPerson && GetController() != nullptr);
	}

	// Free-look carries over between views and would otherwise leave the cockpit
	// facing sideways the moment the player switched in.
	LookYawOffset = 0.f;
	LookPitchOffset = 0.f;

	if (CockpitPivot)
	{
		CockpitPivot->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (CockpitCamera)
	{
		CockpitCamera->SetRelativeRotation(FRotator::ZeroRotator);
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
		? FMath::Clamp(Movement->Velocity.Size() / FlightMaxSpeed, 0.f, 1.f)
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
	// Consumed by UpdateFlight when it runs; dropped when a scripted arrival or
	// the deployment hold ran instead, so it does not pile up and unload later.
	PendingTurn = FVector2D::ZeroVector;
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

	Movement->Velocity = FMath::VInterpConstantTo(Movement->Velocity, Target, DeltaSeconds, FlightAcceleration);
	ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.6f, DeltaSeconds, 4.f);

	Movement->MoveByVelocity(DeltaSeconds);

	// Arrived means settled, not merely passing through: a craft still doing
	// 40 m/s through the radius has not stopped to collect anyone.
	bScriptedArrived = (Distance <= ScriptedArriveRadius) && (Movement->Velocity.Size() < 400.f);
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
	UpdateRotation(DeltaSeconds, HoverYawRate);

	// Build the desired direction in the craft's own frame, all three axes. In
	// flight that is six degrees of freedom: nose down and "up" is along the
	// canopy. Hover levels the hull, so there up is up.
	FVector Desired = FVector::ZeroVector;
	Desired += GetActorForwardVector() * MoveInput.X;
	Desired += GetActorRightVector() * MoveInput.Y;
	Desired += GetActorUpVector() * MoveInput.Z;

	if (Desired.IsNearlyZero() && bStationKeepEnabled)
	{
		// Assist: with no pilot input, ease onto the deployment point instead of
		// simply stopping wherever the craft happened to drift to.
		const FVector ToTarget = StationKeepTarget - GetActorLocation();
		const FVector Assist = (ToTarget * StationKeepGain).GetClampedToMaxSize(StationKeepMaxSpeed);
		Movement->Velocity = FMath::VInterpConstantTo(Movement->Velocity, Assist, DeltaSeconds, Accel);
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.25f, DeltaSeconds, 3.f);
	}
	else if (Desired.IsNearlyZero())
	{
		Movement->Velocity = FMath::VInterpConstantTo(Movement->Velocity, FVector::ZeroVector, DeltaSeconds, Braking);
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 0.f, DeltaSeconds, 3.f);
	}
	else
	{
		const FVector Target = Desired.GetClampedToMaxSize(1.f) * MaxSpeed;
		Movement->Velocity = FMath::VInterpConstantTo(Movement->Velocity, Target, DeltaSeconds, Accel);
		ThrustAlpha = FMath::FInterpTo(ThrustAlpha, 1.f, DeltaSeconds, 6.f);
	}

	// Bleed down to the current maximum at the braking rate rather than clamping
	// hard. A hard clamp would make HoverBlendTime dictate the whole deceleration
	// when entering hover at speed, bypassing the braking model entirely.
	const float Speed = Movement->Velocity.Size();
	if (Speed > MaxSpeed)
	{
		const float Shed = FMath::Max(MaxSpeed, Speed - Braking * DeltaSeconds);
		Movement->Velocity = Movement->Velocity.GetSafeNormal() * Shed;
	}

	Movement->MoveByVelocity(DeltaSeconds);
}

void AEOAircraftPawn::UpdateRotation(float DeltaSeconds, float YawRate)
{
	const FQuat Before = GetActorQuat();

	// Everything is applied in the craft's own frame: that is what makes a pull
	// on the mouse while rolled ninety degrees a turn rather than a climb.
	FRotator Local = FRotator::ZeroRotator;
	Local.Yaw = PendingTurn.X;
	Local.Pitch = PendingTurn.Y * FlightPitchScale;
	Local.Roll = RollInput * FlightRollRate * DeltaSeconds;
	Local.Yaw += YawInput * YawRate * DeltaSeconds;
	if (!Local.IsNearlyZero())
	{
		AddActorLocalRotation(Local.Quaternion());
	}
	PendingTurn = FVector2D::ZeroVector;

	// Hover sits flat: the chase camera, the deployment socket and the station
	// keep all assume the hull is level, so engaging hover eases pitch and roll
	// out while keeping the heading.
	if (bHoverRequested)
	{
		const FQuat Now = GetActorQuat();
		const FQuat Level = FRotator(0.f, Now.Rotator().Yaw, 0.f).Quaternion();
		if (!Now.Equals(Level, 1e-4f))
		{
			SetActorRotation(FMath::QInterpTo(Now, Level, DeltaSeconds, HoverLevelSpeed));
		}
	}

	// Carry momentum around with the nose. Without this the craft keeps its old
	// world heading through a turn and slides sideways like it is on ice.
	const FQuat Delta = GetActorQuat() * Before.Inverse();
	if (!Delta.IsIdentity(1e-6f))
	{
		Movement->Velocity = FQuat::Slerp(FQuat::Identity, Delta, VelocityTurnFactor)
			.RotateVector(Movement->Velocity);
	}
}

FVector AEOAircraftPawn::GetVelocity() const
{
	// Overridden so that everything reading a pawn's velocity - feedback scaling,
	// the HUD, the self-tests - sees the flight model's value.
	return Movement ? Movement->Velocity : FVector::ZeroVector;
}

void AEOAircraftPawn::ReportImpact(const FHitResult& Hit)
{
	UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this);
	if (!Feedback)
	{
		return;
	}

	// How much of the impact went into the surface rather than along it. A glancing
	// clip at speed is a scrape; flying into a wall head-on is a collision. Using
	// the normal component rather than raw speed is what keeps those distinct.
	const float ClosingSpeed = FMath::Abs(FVector::DotProduct(Movement->Velocity, Hit.Normal));
	const float Severity = FMath::Clamp(ClosingSpeed / FMath::Max(FlightMaxSpeed * 0.35f, 1.f), 0.f, 1.5f);

	FEOFeedbackContext Context = FEOFeedbackContext::At(Hit.ImpactPoint);
	Context.Rotation = Hit.ImpactNormal.Rotation();
	Context.Scale = Severity;

	if (Severity >= 0.35f)
	{
		Feedback->Play(EOFeedbackEvents::Aircraft_Collision, Context);
	}
	else if (ScrapeCooldown <= 0.f)
	{
		// Rate-limited: a craft sliding along a wall generates a contact every
		// frame, and a scrape one-shot per frame is a buzzsaw.
		ScrapeCooldown = 0.25f;
		Context.Scale = FMath::Max(Severity, 0.3f);
		Feedback->Play(EOFeedbackEvents::Aircraft_Scrape, Context);
	}
}

void AEOAircraftPawn::UpdateAttitude(float DeltaSeconds)
{
	if (!HullPivot)
	{
		return;
	}

	// A slight lean into a strafe or a turn, and a nose-down under thrust, so a
	// hovering craft reads as working. Hover only: in flight the airframe
	// itself rolls and pitches under the mouse, and a lean on top of a real
	// attitude reads as slop.
	const float Lean = HoverBlend * 0.25f;
	const float TargetRoll = FMath::Clamp(MoveInput.Y + YawInput, -1.f, 1.f) * MaxBankAngle * Lean;
	const float TargetPitch = -MoveInput.X * MaxPitchAngle * Lean;

	const FRotator Current = HullPivot->GetRelativeRotation();
	const FRotator Target(TargetPitch, 0.f, TargetRoll);
	HullPivot->SetRelativeRotation(
		FMath::RInterpTo(Current, Target, DeltaSeconds, AttitudeInterpSpeed));
}

void AEOAircraftPawn::UpdateLook(float DeltaSeconds)
{
	// Let go of the mouse and the camera swings back behind the craft, so the
	// default view is always the one you fly with. In the cockpit the same offsets
	// turn the pilot's head instead of swinging a boom.
	if (!bLookActiveThisFrame)
	{
		LookYawOffset = FMath::FInterpTo(LookYawOffset, 0.f, DeltaSeconds, LookRecenterSpeed);
		LookPitchOffset = FMath::FInterpTo(LookPitchOffset, 0.f, DeltaSeconds, LookRecenterSpeed);
	}

	const FRotator Offset(LookPitchOffset, LookYawOffset, 0.f);

	if (bFirstPerson)
	{
		// The camera turns inside the cockpit, not with it. CockpitMesh is a child
		// of CockpitPivot, so rotating the pivot swung the whole interior round
		// with the view - the canopy frames and console moved with your head,
		// which reads as the aircraft yawing rather than the pilot looking.
		if (CockpitCamera)
		{
			CockpitCamera->SetRelativeRotation(Offset);
		}
		return;
	}

	if (CameraBoom)
	{
		CameraBoom->SetRelativeRotation(Offset);
	}
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

	if (bFirstPerson && CockpitCamera)
	{
		// The cockpit widens far less. Inside a canopy the frame is fixed in view,
		// so a large FOV swing reads as the canopy warping rather than as speed.
		float CockpitImpulse = 0.f;
		if (const UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
		{
			CockpitImpulse = Feedback->GetFOVImpulse() * CockpitFOVImpulseScale;
		}

		const float TargetCockpitFOV = FMath::Lerp(CockpitFOV, CockpitFOVFast, SpeedAlpha);
		SmoothedCockpitFOV = FMath::FInterpTo(
			SmoothedCockpitFOV > 0.f ? SmoothedCockpitFOV : TargetCockpitFOV,
			TargetCockpitFOV, DeltaSeconds, 3.f);

		CockpitCamera->SetFieldOfView(FMath::Clamp(SmoothedCockpitFOV + CockpitImpulse, 60.f, 130.f));
	}

	// The steady-state FOV eases; the feedback impulse is added on top afterwards
	// so a braking kick is not smoothed away to nothing before it is visible.
	const float TargetFOV = FMath::Lerp(CameraFOVBase, CameraFOVFast, SpeedAlpha);
	SmoothedFOV = FMath::FInterpTo(SmoothedFOV > 0.f ? SmoothedFOV : TargetFOV, TargetFOV,
		DeltaSeconds, 3.f);

	float Impulse = 0.f;
	if (const UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this))
	{
		Impulse = Feedback->GetFOVImpulse();
	}

	ChaseCamera->SetFieldOfView(FMath::Clamp(SmoothedFOV + Impulse, 60.f, 140.f));
}

void AEOAircraftPawn::UpdateFeedback(float DeltaSeconds)
{
	UpdateFlightTransients(DeltaSeconds);

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

void AEOAircraftPawn::UpdateFlightTransients(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	ScrapeCooldown = FMath::Max(0.f, ScrapeCooldown - DeltaSeconds);
	TransientCooldown = FMath::Max(0.f, TransientCooldown - DeltaSeconds);

	UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this);
	if (!Feedback)
	{
		PreviousSpeed = Movement->Velocity.Size();
		return;
	}

	const float Speed = Movement->Velocity.Size();
	const float Along = (Speed - PreviousSpeed) / DeltaSeconds;
	PreviousSpeed = Speed;

	// One-shots fire on the rate of change, not on the input: the craft carries
	// momentum, so "the player pressed forward" and "the craft actually surged"
	// are different moments, and only the second one is worth hearing.
	if (TransientCooldown <= 0.f)
	{
		const float SurgeThreshold = FlightAcceleration * 0.75f;
		const float BrakeThreshold = -FlightBraking * 0.9f;

		FEOFeedbackContext Context = FEOFeedbackContext::AtActor(this);
		Context.AttachTo = HullPivot;

		if (Along > SurgeThreshold && GetSpeedAlpha() > 0.25f)
		{
			Context.Scale = FMath::Clamp(Along / FMath::Max(FlightAcceleration, 1.f), 0.5f, 1.5f);
			Feedback->Play(EOFeedbackEvents::Aircraft_Accelerate, Context);
			TransientCooldown = 0.9f;
		}
		else if (Along < BrakeThreshold && PreviousSpeed > FlightMaxSpeed * 0.2f)
		{
			Context.Scale = FMath::Clamp(-Along / FMath::Max(FlightBraking, 1.f), 0.5f, 1.5f);
			Feedback->Play(EOFeedbackEvents::Aircraft_BrakeHard, Context);
			TransientCooldown = 0.7f;
		}
		else if (FMath::Abs(MoveInput.Y) > 0.5f && FMath::Abs(PreviousLateralInput) < 0.2f)
		{
			// Strafe started from neutral: the lateral thrusters just fired.
			Context.Scale = 1.f;
			Feedback->Play(EOFeedbackEvents::Aircraft_LateralBurst, Context);
			TransientCooldown = 0.5f;
		}
	}

	PreviousLateralInput = MoveInput.Y;
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
	Input->BindAction(Config->FlightRollAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Roll);
	// Completed zeroes the held demand; without it the throttle would stick on release.
	Input->BindAction(Config->FlightMoveAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_MoveReleased);
	Input->BindAction(Config->FlightVerticalAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_VerticalReleased);
	Input->BindAction(Config->FlightRollAction, ETriggerEvent::Completed, this, &AEOAircraftPawn::Input_RollReleased);
	// Started, not Triggered: a tap steps the throttle once, however long it is held.
	Input->BindAction(Config->ThrottleUpAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_ThrottleUp);
	Input->BindAction(Config->ThrottleDownAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_ThrottleDown);

	Input->BindAction(Config->LookAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_Look);
	Input->BindAction(Config->LookStickAction, ETriggerEvent::Triggered, this, &AEOAircraftPawn::Input_LookStick);
	Input->BindAction(Config->HoverAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_HoverToggle);
	Input->BindAction(Config->DeployAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_Deploy);
	Input->BindAction(Config->ToggleMapAction, ETriggerEvent::Started, this, &AEOAircraftPawn::Input_ToggleMap);
}

void AEOAircraftPawn::ApplyKeyDemand()
{
	// W/S are held thrust in hover and a tapped throttle in flight. A/D strafe
	// in flight and turn in hover; Q/E roll in flight and strafe in hover. The
	// slot a key does not own in this mode is zeroed, or a strafe held through
	// the toggle would carry on as a turn.
	bKeyDriven = true;
	if (bHoverRequested)
	{
		MoveInput.X = HeldMove.Y;
		MoveInput.Y = HeldRoll;
		YawInput = HeldMove.X;
		RollInput = 0.f;
	}
	else
	{
		MoveInput.X = ThrottleFraction();
		MoveInput.Y = HeldMove.X;
		YawInput = 0.f;
		RollInput = HeldRoll;
	}
}

float AEOAircraftPawn::ThrottleFraction() const
{
	// Expressed against the fast speed because that is what UpdateFlight scales
	// the demand by; the slow detent is a fraction of it.
	if (FlightMaxSpeed <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	switch (ThrottleStep)
	{
	case 3:  return 1.f;
	case 2:  return FlightSlowSpeed / FlightMaxSpeed;
	case 1:  return FlightCreepSpeed / FlightMaxSpeed;
	default: return 0.f;
	}
}

void AEOAircraftPawn::Input_ThrottleUp(const FInputActionValue& Value)
{
	// In hover the same key is held thrust, handled by Input_Move.
	if (bHoverRequested)
	{
		return;
	}
	ThrottleStep = FMath::Min(ThrottleStep + 1, MaxThrottleStep);
	ApplyKeyDemand();
}

void AEOAircraftPawn::Input_ThrottleDown(const FInputActionValue& Value)
{
	if (bHoverRequested)
	{
		return;
	}
	// Stops at zero: there is no reverse in flight. Turn round.
	ThrottleStep = FMath::Max(ThrottleStep - 1, 0);
	ApplyKeyDemand();
}

void AEOAircraftPawn::Input_Move(const FInputActionValue& Value)
{
	HeldMove = Value.Get<FVector2D>();
	ApplyKeyDemand();
}

void AEOAircraftPawn::Input_MoveReleased(const FInputActionValue& Value)
{
	HeldMove = FVector2D::ZeroVector;
	ApplyKeyDemand();
}

void AEOAircraftPawn::Input_Vertical(const FInputActionValue& Value)
{
	MoveInput.Z = Value.Get<float>();
}

void AEOAircraftPawn::Input_VerticalReleased(const FInputActionValue& Value)
{
	MoveInput.Z = 0.f;
}

void AEOAircraftPawn::Input_Roll(const FInputActionValue& Value)
{
	HeldRoll = Value.Get<float>();
	ApplyKeyDemand();
}

void AEOAircraftPawn::Input_RollReleased(const FInputActionValue& Value)
{
	HeldRoll = 0.f;
	ApplyKeyDemand();
}

void AEOAircraftPawn::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}

	// Inversion is applied here rather than baked into the mapping, so that
	// changing the setting takes effect without rebuilding the context.
	const FVector2D Signed(Axis.X, Axis.Y * UEOInputSettings::MouseSign());

	// The same mouse turns the craft in flight and the head in hover. A mouse
	// reports a delta, already frame-independent, so it is degrees directly.
	if (bHoverRequested)
	{
		ApplyLookDelta(Signed * LookSensitivity);
	}
	else
	{
		AddTurnDelta(Signed * FlightTurnSensitivity);
	}
}

void AEOAircraftPawn::Input_LookStick(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}

	// A stick reports a held position, so it is a rate: scale by delta time or
	// the camera whips round faster the better the frame rate.
	const FVector2D Signed(Axis.X, Axis.Y * UEOInputSettings::StickSign());
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	if (bHoverRequested)
	{
		ApplyLookDelta(Signed * StickLookRate * DeltaSeconds);
	}
	else
	{
		AddTurnDelta(Signed * FlightStickTurnRate * DeltaSeconds);
	}
}

void AEOAircraftPawn::AddTurnDelta(const FVector2D& Delta)
{
	PendingTurn += Delta;
}

void AEOAircraftPawn::ApplyLookDelta(const FVector2D& Delta)
{
	// Free-look is a bounded offset on the boom rather than controller rotation,
	// because the controller must not be allowed to rotate the hull.
	//
	// The cockpit gets much tighter limits: a seated pilot can glance around the
	// canopy, not turn their head through the bulkhead behind them. Without this,
	// first-person free-look swings the view straight out through the fuselage.
	const float YawLimit = bFirstPerson ? CockpitMaxLookYaw : MaxLookYaw;
	const float PitchLimit = bFirstPerson ? CockpitMaxLookPitch : MaxLookPitch;

	LookYawOffset = FMath::Clamp(LookYawOffset + Delta.X, -YawLimit, YawLimit);
	LookPitchOffset = FMath::Clamp(LookPitchOffset + Delta.Y, -PitchLimit, PitchLimit);
	bLookActiveThisFrame = true;
}

void AEOAircraftPawn::Input_HoverToggle(const FInputActionValue& Value)
{
	Execute_SetHoverEnabled(this, !bHoverRequested);
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

	// Whether the hull is hidden depends on someone actually sitting in the
	// cockpit, so it has to be re-evaluated whenever that changes.
	ApplyViewMode();
}

void AEOAircraftPawn::UnPossessed()
{
	// Hand the HUD back before losing the controller, or it keeps drawing into a
	// panel the player can no longer see and the operative gets a blank screen.
	if (APlayerController* OwningController = Cast<APlayerController>(GetController()))
	{
		if (AEODebugHUD* Hud = Cast<AEODebugHUD>(OwningController->GetHUD()))
		{
			Hud->SetProjectionScreen(nullptr);
		}
	}

	Super::UnPossessed();

	// Removing the mapping context can tear an in-progress action down without a
	// Completed event, which would otherwise leave a key held forever. The mode
	// is not a key: a craft left in hover stays in hover.
	ClearControlDemand();

	ApplyViewMode();
}

void AEOAircraftPawn::ClearControlDemand()
{
	MoveInput = FVector::ZeroVector;
	YawInput = 0.f;
	RollInput = 0.f;
	HeldMove = FVector2D::ZeroVector;
	HeldRoll = 0.f;
	ThrottleStep = 0;
	PendingTurn = FVector2D::ZeroVector;
	bKeyDriven = false;
}

void AEOAircraftPawn::SetFlightInput_Implementation(const FVector& InMoveInput)
{
	MoveInput = InMoveInput;
	bKeyDriven = false;
}

void AEOAircraftPawn::SetYawInput_Implementation(float InYawInput)
{
	YawInput = InYawInput;
	bKeyDriven = false;
}

void AEOAircraftPawn::SetHoverEnabled_Implementation(bool bEnabled)
{
	if (bHoverRequested == bEnabled)
	{
		return;
	}
	bHoverRequested = bEnabled;

	// The view is part of the mode: flight is flown from the cockpit, hover from
	// outside, where the drop can be lined up.
	SetFirstPerson(!bEnabled);

	// Hover is the stopped detent: the throttle is wound back so the craft
	// parks, and coming back to flight starts from a standstill.
	ThrottleStep = 0;

	// Keys held through the toggle are reinterpreted for the new mode. Only
	// when the keys own the demand: one set from code - the tests and the
	// approach - survives a mode change the same code asked for.
	if (bKeyDriven)
	{
		ApplyKeyDemand();
	}
}

bool AEOAircraftPawn::IsHovering_Implementation() const
{
	return bHoverRequested;
}

float AEOAircraftPawn::GetCurrentSpeed_Implementation() const
{
	return Movement->Velocity.Size();
}

FTransform AEOAircraftPawn::GetDeploymentSocketTransform_Implementation() const
{
	return DeploymentSocket->GetComponentTransform();
}

bool AEOAircraftPawn::IsReadyForDeployment_Implementation() const
{
	// M1 proxy for "stabilised over the site": committed to hover and nearly stopped.
	// M3 replaces this with the hover volume test.
	return bHoverRequested && Movement->Velocity.Size() < 200.f;
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
		Movement->Velocity = FVector::ZeroVector;
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
	ClearControlDemand();
	Movement->Velocity = FVector::ZeroVector;
	Execute_SetHoverEnabled(this, false);
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

	// Flight may have left the hull at any attitude; a reset is level.
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}
