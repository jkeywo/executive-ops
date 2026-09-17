#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "EOCameraShakes.generated.h"

/**
 * A directional kick that decays, rather than a noise field.
 *
 * The brief bans one generic shake reused everywhere, and bans continuous random
 * screenshake outright. What the verbs actually need is an impulse with a
 * direction and a decay: recoil pushes the view up and back, a landing compresses
 * it downward, a collision throws it along the impact normal. So the pattern is
 * a damped oscillation around a chosen axis, not Perlin noise.
 *
 * Written here rather than using the EngineCameras plugin's oscillator patterns
 * so the module keeps its current dependency set.
 */
UCLASS(EditInlineNew)
class EXECUTIVEOPS_API UEOImpulseShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UEOImpulseShakePattern(const FObjectInitializer& ObjectInitializer);

	/** Seconds from full amplitude to nothing. Short: these are transients. */
	UPROPERTY(EditAnywhere, Category = "Impulse", meta = (ClampMin = "0.01"))
	float Duration = 0.3f;

	/** Rotational kick in degrees: pitch, yaw, roll. */
	UPROPERTY(EditAnywhere, Category = "Impulse")
	FRotator RotationAmplitude = FRotator(1.2f, 0.4f, 0.6f);

	UPROPERTY(EditAnywhere, Category = "Impulse")
	FVector LocationAmplitude = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Impulse")
	float FOVAmplitude = 0.f;

	/** Oscillations per second. Low values read as a shove, high as a rattle. */
	UPROPERTY(EditAnywhere, Category = "Impulse", meta = (ClampMin = "0.1"))
	float Frequency = 9.f;

	/**
	 * 1 = pure one-directional shove that eases back to centre (recoil, landing).
	 * 0 = symmetric oscillation either side of centre (collision rattle).
	 */
	UPROPERTY(EditAnywhere, Category = "Impulse", meta = (ClampMin = "0", ClampMax = "1"))
	float Directionality = 1.f;

private:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params,
		FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;

	float Elapsed = 0.f;
};

/** Base for the tuned presets below: builds the pattern and exposes it. */
UCLASS(Abstract)
class EXECUTIVEOPS_API UEOCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UEOCameraShake(const FObjectInitializer& ObjectInitializer);

protected:
	UEOImpulseShakePattern* GetImpulsePattern() const { return Impulse; }

private:
	UPROPERTY()
	TObjectPtr<UEOImpulseShakePattern> Impulse;
};

/** Pistol recoil: up and slightly right, gone before the next shot is allowed. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_PistolRecoil : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_PistolRecoil(const FObjectInitializer& ObjectInitializer);
};

/** Takedown contact: one hard lateral snap, no ring-out. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_TakedownImpact : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_TakedownImpact(const FObjectInitializer& ObjectInitializer);
};

/** Hard landing: downward compression with a small settle. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_HardLanding : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_HardLanding(const FObjectInitializer& ObjectInitializer);
};

/** Deployment launch: the one FOV-led shake, because it is a signature moment. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_DeployLaunch : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_DeployLaunch(const FObjectInitializer& ObjectInitializer);
};

/** Aircraft collision: the only genuinely rattly shake in the set. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_AircraftCollision : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_AircraftCollision(const FObjectInitializer& ObjectInitializer);
};

/** Taking a hit: small and short. The player needs to see where to run. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_PlayerDamage : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_PlayerDamage(const FObjectInitializer& ObjectInitializer);
};

/** Boarding the aircraft on extraction: a firm mechanical thump. */
UCLASS()
class EXECUTIVEOPS_API UEOShake_ExtractionBoard : public UEOCameraShake
{
	GENERATED_BODY()
public:
	UEOShake_ExtractionBoard(const FObjectInitializer& ObjectInitializer);
};
