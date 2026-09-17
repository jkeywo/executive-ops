#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EOFeedbackSettings.generated.h"

class UEOFeedbackPresetSet;

/** Camera shake allowance. Reduced halves every shake; Off removes them entirely. */
UENUM(BlueprintType)
enum class EEOCameraShakeMode : uint8
{
	Off			UMETA(DisplayName = "Off"),
	Reduced		UMETA(DisplayName = "Reduced"),
	Full		UMETA(DisplayName = "Full")
};

/** Screen-effect allowance: vignettes, FOV impulses, directional flashes. */
UENUM(BlueprintType)
enum class EEOScreenEffectMode : uint8
{
	Off			UMETA(DisplayName = "Off"),
	Reduced		UMETA(DisplayName = "Reduced"),
	Full		UMETA(DisplayName = "Full")
};

/**
 * Feedback wiring and the accessibility controls that scale it.
 *
 * The brief's §15 requires these to exist before the effects do, not after, so
 * the subsystem consults them on every event rather than having per-verb opt-outs
 * retrofitted later. Config-backed, so the eventual settings menu only has to
 * write the same values.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Executive Ops - Feedback"))
class EXECUTIVEOPS_API UEOFeedbackSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** The preset set the feedback subsystem plays from. */
	UPROPERTY(config, EditAnywhere, Category = "Feedback")
	TSoftObjectPtr<UEOFeedbackPresetSet> PresetSet;

	UPROPERTY(config, EditAnywhere, Category = "Accessibility")
	EEOCameraShakeMode CameraShake = EEOCameraShakeMode::Full;

	UPROPERTY(config, EditAnywhere, Category = "Accessibility")
	EEOScreenEffectMode ScreenEffects = EEOScreenEffectMode::Full;

	UPROPERTY(config, EditAnywhere, Category = "Accessibility")
	bool bHapticsEnabled = true;

	UPROPERTY(config, EditAnywhere, Category = "Accessibility")
	bool bMotionBlurEnabled = false;

	/** Master volume trim for feedback one-shots, separate from the mix. */
	UPROPERTY(config, EditAnywhere, Category = "Accessibility", meta = (ClampMin = "0", ClampMax = "2"))
	float FeedbackVolume = 1.f;

	float GetCameraShakeScale() const
	{
		switch (CameraShake)
		{
		case EEOCameraShakeMode::Off:		return 0.f;
		case EEOCameraShakeMode::Reduced:	return 0.5f;
		default:							return 1.f;
		}
	}

	float GetScreenEffectScale() const
	{
		switch (ScreenEffects)
		{
		case EEOScreenEffectMode::Off:		return 0.f;
		case EEOScreenEffectMode::Reduced:	return 0.5f;
		default:							return 1.f;
		}
	}

	static const UEOFeedbackSettings& Get()
	{
		return *GetDefault<UEOFeedbackSettings>();
	}
};
