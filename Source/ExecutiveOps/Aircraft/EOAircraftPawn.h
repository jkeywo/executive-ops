#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "EOAircraftPawn.generated.h"

class UAudioComponent;
class UBoxComponent;
class UCameraComponent;
class USceneComponent;
class USoundBase;
class USpringArmComponent;
class UStaticMeshComponent;
class UEOInputConfig;
struct FInputActionValue;

/**
 * Greybox VTOL.
 *
 * Two handling modes sharing one velocity: HOVER is slow, heavily damped and
 * station-keeping, for working over a mission site; FLIGHT is fast and carries
 * momentum, for crossing the city. The blend between them is continuous rather
 * than a hard switch, because the transition is the part that has to feel good.
 *
 * Movement is swept against world geometry each frame. There is no physics
 * simulation: velocity is integrated directly, which keeps handling entirely
 * predictable and cheap to tune.
 */
UCLASS()
class EXECUTIVEOPS_API AEOAircraftPawn : public APawn, public IEOAircraftControlInterface
{
	GENERATED_BODY()

public:
	AEOAircraftPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetVelocity() const override;

	//~ IEOAircraftControlInterface
	virtual void SetFlightInput_Implementation(const FVector& MoveInput) override;
	virtual void SetYawInput_Implementation(float YawInput) override;
	virtual void SetHoverEnabled_Implementation(bool bEnabled) override;
	virtual bool IsHovering_Implementation() const override;
	virtual float GetCurrentSpeed_Implementation() const override;
	virtual FTransform GetDeploymentSocketTransform_Implementation() const override;
	virtual bool IsReadyForDeployment_Implementation() const override;
	virtual void ResetFlightState_Implementation() override;
	virtual void SetStationKeepTarget_Implementation(const FVector& WorldLocation, bool bEnabled) override;
	virtual void SetDeploymentHold_Implementation(bool bHeld) override;
	virtual void SetScriptedDestination_Implementation(const FVector& WorldLocation, bool bEnabled) override;
	virtual bool HasReachedScriptedDestination_Implementation() const override;
	//~ End IEOAircraftControlInterface

	/** 0 = fully in flight mode, 1 = fully hovering. Drives handling and feedback. */
	UFUNCTION(BlueprintPure, Category = "Aircraft")
	float GetHoverBlend() const { return HoverBlend; }

	/** Current speed as a fraction of the flight-mode maximum. */
	UFUNCTION(BlueprintPure, Category = "Aircraft")
	float GetSpeedAlpha() const;

	/** How hard the engines are working, 0..1. Drives thruster scale and audio pitch. */
	UFUNCTION(BlueprintPure, Category = "Aircraft")
	float GetThrustAlpha() const { return ThrustAlpha; }

	/** True when flying from the cockpit rather than the chase camera. */
	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsFirstPerson() const { return bFirstPerson; }

	/**
	 * True while a HUD panel is showing on the cockpit glass - whichever of the
	 * two is live. What the flight suite asserts on when it toggles the view,
	 * since the panels themselves are the pawn's business.
	 */
	UFUNCTION(BlueprintPure, Category = "Aircraft|Cockpit")
	bool IsCockpitPanelShowing() const;

	/** True once the widget panel has a widget and has taken over from the old one. */
	UFUNCTION(BlueprintPure, Category = "Aircraft|Cockpit")
	bool IsWidgetPanelLive() const;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetFirstPerson(bool bNewFirstPerson);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ToggleView();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Input_Move(const FInputActionValue& Value);
	void Input_MoveReleased(const FInputActionValue& Value);
	void Input_Vertical(const FInputActionValue& Value);
	void Input_VerticalReleased(const FInputActionValue& Value);
	void Input_Yaw(const FInputActionValue& Value);
	void Input_YawReleased(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_LookStick(const FInputActionValue& Value);
	void Input_HoverStart(const FInputActionValue& Value);
	void Input_HoverStop(const FInputActionValue& Value);
	void Input_Deploy(const FInputActionValue& Value);
	void Input_ToggleMap(const FInputActionValue& Value);
	void Input_ToggleView(const FInputActionValue& Value);

	/** Activates the right camera and hides whatever the other view should not see. */
	void ApplyViewMode();

	/** Integrate velocity from the current input, then sweep the actor through the world. */
	void UpdateFlight(float DeltaSeconds);

	/** Flies itself to ScriptedDestination. Returns true while it is doing so. */
	bool UpdateScriptedArrival(float DeltaSeconds);

	/** Bank, pitch and settle the hull to communicate what the craft is doing. */
	void UpdateAttitude(float DeltaSeconds);

	/** Chase camera distance and FOV respond to speed so fast flight reads as fast. */
	void UpdateCamera(float DeltaSeconds);

	/** Thruster scale and engine audio. */
	void UpdateFeedback(float DeltaSeconds);

	/**
	 * Watches speed and input for the moments worth a one-shot: a surge, a hard
	 * brake, a lateral burst. Rate-limited, because these are punctuation.
	 */
	void UpdateFlightTransients(float DeltaSeconds);

	/** Turns a swept blocking hit into either a collision or a scrape. */
	void ReportImpact(const FHitResult& Hit);

	/** Free-look offset on the chase boom, recentring when the player lets go. */
	void UpdateLook(float DeltaSeconds);

	/** Shared by mouse and stick look, which differ only in how they are scaled. */
	void ApplyLookDelta(const FVector2D& Delta);

	/** Zero the held control demand. Called whenever possession changes. */
	void ClearControlDemand();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UBoxComponent> CollisionBox;

	/**
	 * Owns this frame's velocity and the move it produces. The pawn decides what
	 * the craft is trying to do; this turns that into a position.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<class UEOAircraftMovementComponent> Movement;

	/** Everything visual hangs off here so the hull can bank without rotating collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<USceneComponent> HullPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UStaticMeshComponent> ThrusterLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UStaticMeshComponent> ThrusterRight;

	/** The single fixed deployment socket referenced by M3. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft|Deployment")
	TObjectPtr<USceneComponent> DeploymentSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> ChaseCamera;

	/** Carries the cockpit interior and the pilot's viewpoint, and banks with the hull. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> CockpitPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UStaticMeshComponent> CockpitMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CockpitCamera;

	/**
	 * The HUD, on glass in front of the pilot.
	 *
	 * Attached to CockpitPivot rather than the camera, so the readouts belong to
	 * the airframe: looking around pans across a fixed display instead of
	 * dragging it along with your head.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<class UEOHudScreenComponent> HudScreen;

	/**
	 * The HUD on the cockpit glass, as a widget on a curved surface, driven by
	 * the same view model as the viewport. Takes over from HudScreen the moment
	 * a Panel Widget class is assigned in the HUD settings; until then it has no
	 * widget and the old panel keeps drawing.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft|Cockpit")
	TObjectPtr<class UEOHudPanelComponent> HudPanel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UAudioComponent> EngineAudio;

	// ---- Handling: flight mode -------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Flight", meta = (ClampMin = "0"))
	float FlightMaxSpeed = 7000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Flight", meta = (ClampMin = "0"))
	float FlightAcceleration = 5000.f;

	/** Deceleration when the player releases input. Low, so flight carries momentum. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Flight", meta = (ClampMin = "0"))
	float FlightBraking = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Flight", meta = (ClampMin = "0"))
	float FlightYawRate = 65.f;

	/**
	 * How much of the craft's momentum follows the nose when it yaws.
	 * 1 = no sideslip at all, 0 = momentum is purely world-space and the craft
	 * drifts sideways through a turn.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float VelocityTurnFactor = 0.85f;

	/** How aggressively a scripted arrival slows as it closes on its destination. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Extraction", meta = (ClampMin = "0.01"))
	float ScriptedApproachGain = 1.4f;

	/** Within this of the destination, and slow, the arrival counts as complete. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Extraction", meta = (ClampMin = "1"))
	float ScriptedArriveRadius = 300.f;

	// ---- Handling: hover mode --------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0"))
	float HoverMaxSpeed = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0"))
	float HoverAcceleration = 4000.f;

	/** High, so releasing input parks the craft rather than drifting it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0"))
	float HoverBraking = 5000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0"))
	float HoverYawRate = 90.f;

	/** How hard station-keeping pulls the craft onto the deployment point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0"))
	float StationKeepGain = 1.8f;

	/** Assist speed cap, so it guides the craft rather than snatching it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0"))
	float StationKeepMaxSpeed = 700.f;

	/** Seconds to blend between the two modes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Hover", meta = (ClampMin = "0.01"))
	float HoverBlendTime = 0.35f;

	// ---- Attitude --------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Attitude")
	float MaxBankAngle = 28.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Attitude")
	float MaxPitchAngle = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Attitude", meta = (ClampMin = "0.01"))
	float AttitudeInterpSpeed = 4.f;

	// ---- Camera ----------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraDistanceHover = 1100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraDistanceFast = 1900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraFOVBase = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraFOVFast = 108.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

	// ---- Cockpit ----------------------------------------------------------------

	/**
	 * Piloting view. First person by default: the cockpit is the point of the
	 * aircraft, and the chase camera is the concession, not the other way round.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit")
	bool bFirstPerson = true;

	/** Hide the exterior hull while inside it. Off if the model has a real interior. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit")
	bool bHideHullInCockpit = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit")
	float CockpitFOV = 95.f;

	/** Deliberately a narrow range: a canopy that warps with speed reads as a bug. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit")
	float CockpitFOVFast = 103.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit", meta = (ClampMin = "0", ClampMax = "1"))
	float CockpitFOVImpulseScale = 0.4f;

	/** How far the pilot may glance either side before the canopy frame stops them. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit", meta = (ClampMin = "0", ClampMax = "90"))
	float CockpitMaxLookYaw = 32.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Cockpit", meta = (ClampMin = "0", ClampMax = "90"))
	float CockpitMaxLookPitch = 18.f;

	/** Degrees per second at full stick deflection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float StickLookRate = 140.f;

	/** Free-look limits. The boom recentres behind the craft when the mouse stops. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float MaxLookYaw = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float MaxLookPitch = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float LookRecenterSpeed = 2.5f;

	// ---- Feedback --------------------------------------------------------------

	/** Optional: assign an engine loop in the Blueprint. Silent if unset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> EngineLoopSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	float EnginePitchIdle = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	float EnginePitchMax = 1.6f;

private:
	const UEOInputConfig* GetInputConfig() const;

	/**
	 * Current control demand, held until changed rather than cleared each frame.
	 *
	 * Enhanced Input zeroes it on release (the Completed bindings), which keeps the
	 * throttle stable across frames and lets SetFlightInput drive the craft from
	 * code - the self-test and, later, the scripted extraction approach.
	 */
	FVector MoveInput = FVector::ZeroVector;
	float YawInput = 0.f;

	bool bHoverRequested = false;

	/** Smoothed 0..1 towards bHoverRequested. */
	float HoverBlend = 0.f;

	float ThrustAlpha = 0.f;

	FVector StationKeepTarget = FVector::ZeroVector;
	bool bStationKeepEnabled = false;
	bool bDeploymentHold = false;

	FVector ScriptedDestination = FVector::ZeroVector;
	bool bScriptedArrival = false;
	bool bScriptedArrived = false;

	/** Feedback transient tracking: see UpdateFlightTransients. */
	float PreviousSpeed = 0.f;
	float PreviousLateralInput = 0.f;
	float TransientCooldown = 0.f;
	float ScrapeCooldown = 0.f;

	/** Speed-driven FOV, before the feedback impulse is added on top. */
	float SmoothedFOV = 0.f;
	float SmoothedCockpitFOV = 0.f;

	/** Chase boom free-look, relative to the craft's own heading. */
	float LookYawOffset = 0.f;
	float LookPitchOffset = 0.f;
	bool bLookActiveThisFrame = false;
};
