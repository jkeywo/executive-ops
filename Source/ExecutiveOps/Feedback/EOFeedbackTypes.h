#pragma once

#include "CoreMinimal.h"
#include "EOFeedbackTypes.generated.h"

class AActor;
class UCameraShakeBase;
class UForceFeedbackEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;

/**
 * How loudly an event is allowed to speak.
 *
 * This is not a volume control - it is an editorial one. Signature events are
 * the handful the player should remember (deployment launch, takedown contact);
 * everything else has to stay out of their way. Keeping the category on the
 * preset makes it obvious when a verb has quietly acquired three Strong effects
 * fighting each other.
 */
UENUM(BlueprintType)
enum class EEOFeedbackIntensity : uint8
{
	Subtle		UMETA(DisplayName = "Subtle"),
	Normal		UMETA(DisplayName = "Normal"),
	Strong		UMETA(DisplayName = "Strong"),
	Signature	UMETA(DisplayName = "Signature")
};

/**
 * Everything one gameplay event is allowed to do to the player's senses.
 *
 * Every field is optional. Most presets fill two or three: the brief's rule is
 * the smallest combination that communicates the action, not the largest.
 *
 * Asset references are soft so the module never hard-links third-party content.
 * The packs live outside source control (see Scripts/import_fab_assets.ps1), and
 * a missing pack has to degrade to silence rather than a failed load.
 */
USTRUCT(BlueprintType)
struct FEOFeedbackPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	EEOFeedbackIntensity Intensity = EEOFeedbackIntensity::Normal;

	// ---- Audio -----------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Audio")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Audio", meta = (ClampMin = "0"))
	float VolumeMultiplier = 1.f;

	/** Random pitch spread either side of 1. Cheap variation on repeated one-shots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Audio", meta = (ClampMin = "0", ClampMax = "0.5"))
	float PitchJitter = 0.f;

	// ---- Particles ---------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|VFX")
	TSoftObjectPtr<UNiagaraSystem> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|VFX", meta = (ClampMin = "0.01"))
	float EffectScale = 1.f;

	/** Spawn the effect on the context component rather than at a fixed world point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|VFX")
	bool bAttachEffect = false;

	// ---- Camera ------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Camera")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Camera", meta = (ClampMin = "0"))
	float CameraShakeScale = 1.f;

	/** Degrees of FOV kick. Positive widens (thrust), negative compresses (braking). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Camera")
	float FOVImpulse = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Camera", meta = (ClampMin = "0.01"))
	float FOVImpulseDecay = 0.3f;

	// ---- Screen --------------------------------------------------------------------

	/** 0..1 vignette pulse. Damage and severe danger only - see the brief's §14. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Screen", meta = (ClampMin = "0", ClampMax = "1"))
	float VignetteIntensity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Screen", meta = (ClampMin = "0.01"))
	float VignetteDecay = 0.6f;

	/** Show a directional edge marker pointing at the event. Needs a context direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Screen")
	bool bDirectionalIndicator = false;

	// ---- Haptics ---------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Haptics")
	TSoftObjectPtr<UForceFeedbackEffect> ForceFeedback;

	// ---- World marks -------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Decal")
	TSoftObjectPtr<UMaterialInterface> Decal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Decal")
	FVector DecalSize = FVector(6.f, 6.f, 6.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Decal", meta = (ClampMin = "0"))
	float DecalLifetime = 12.f;

	// ---- Timing ---------------------------------------------------------------------------

	/**
	 * Local hit-stop: briefly slows the context actor only.
	 *
	 * Deliberately not global time dilation. The brief is explicit that perception
	 * mode owns the game's temporal identity later, and that a takedown must not
	 * stop the world every time it happens.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Timing", meta = (ClampMin = "0", ClampMax = "0.5"))
	float HitStopSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback|Timing", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HitStopDilation = 0.25f;
};

/** Where an event happened and what it happened to. */
USTRUCT(BlueprintType)
struct FEOFeedbackContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Feedback")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Feedback")
	FRotator Rotation = FRotator::ZeroRotator;

	/** Attach point for effects and sounds that should travel with something. */
	UPROPERTY(BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<USceneComponent> AttachTo = nullptr;

	/** Who this happened to. Receives any hit-stop. */
	UPROPERTY(BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<AActor> Target = nullptr;

	/** Where the player should look. Drives the directional damage indicator. */
	UPROPERTY(BlueprintReadWrite, Category = "Feedback")
	FVector SourceLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Feedback")
	bool bHasSourceLocation = false;

	/** Scales everything the preset does. Lets one event cover a range of severity. */
	UPROPERTY(BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0"))
	float Scale = 1.f;

	static FEOFeedbackContext At(const FVector& InLocation)
	{
		FEOFeedbackContext Context;
		Context.Location = InLocation;
		return Context;
	}

	static FEOFeedbackContext AtActor(AActor* InActor);
};
