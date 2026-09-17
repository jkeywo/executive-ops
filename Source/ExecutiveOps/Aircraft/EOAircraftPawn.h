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
	virtual FVector GetVelocity() const override { return Velocity; }

	//~ IEOAircraftControlInterface
	virtual void SetFlightInput_Implementation(const FVector& MoveInput) override;
	virtual void SetYawInput_Implementation(float YawInput) override;
	virtual void SetHoverEnabled_Implementation(bool bEnabled) override;
	virtual bool IsHovering_Implementation() const override;
	virtual float GetCurrentSpeed_Implementation() const override;
	virtual FTransform GetDeploymentSocketTransform_Implementation() const override;
	virtual bool IsReadyForDeployment_Implementation() const override;
	virtual void ResetFlightState_Implementation() override;
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
	void Input_HoverStart(const FInputActionValue& Value);
	void Input_HoverStop(const FInputActionValue& Value);
	void Input_Deploy(const FInputActionValue& Value);
	void Input_ToggleMap(const FInputActionValue& Value);

	/** Integrate velocity from the current input, then sweep the actor through the world. */
	void UpdateFlight(float DeltaSeconds);

	/** Bank, pitch and settle the hull to communicate what the craft is doing. */
	void UpdateAttitude(float DeltaSeconds);

	/** Chase camera distance and FOV respond to speed so fast flight reads as fast. */
	void UpdateCamera(float DeltaSeconds);

	/** Thruster scale and engine audio. */
	void UpdateFeedback(float DeltaSeconds);

	/** Free-look offset on the chase boom, recentring when the player lets go. */
	void UpdateLook(float DeltaSeconds);

	/** Zero the held control demand. Called whenever possession changes. */
	void ClearControlDemand();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aircraft")
	TObjectPtr<UBoxComponent> CollisionBox;

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

	/** Fraction of tangential speed kept per second while scraping along a surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aircraft|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float SlideRetentionPerSecond = 0.15f;

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

	FVector Velocity = FVector::ZeroVector;

	bool bHoverRequested = false;

	/** Smoothed 0..1 towards bHoverRequested. */
	float HoverBlend = 0.f;

	float ThrustAlpha = 0.f;

	/** Chase boom free-look, relative to the craft's own heading. */
	float LookYawOffset = 0.f;
	float LookPitchOffset = 0.f;
	bool bLookActiveThisFrame = false;
};
