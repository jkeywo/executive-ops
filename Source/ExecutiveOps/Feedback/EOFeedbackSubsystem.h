#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Feedback/EOFeedbackTypes.h"
#include "EOFeedbackSubsystem.generated.h"

class APlayerController;
class UEOFeedbackPresetSet;

/**
 * One dispatcher for every piece of game feel in the project.
 *
 * Gameplay classes call Play(Pistol_Fire, context) and stop caring. The
 * subsystem owns preset lookup, accessibility scaling, and the transient screen
 * state - FOV impulse, vignette, damage direction - that the camera and HUD read
 * back each frame.
 *
 * Screen state lives here rather than in a component on the controller because
 * two unrelated things need it (the aircraft camera and the HUD) and neither
 * should own it.
 */
UCLASS()
class EXECUTIVEOPS_API UEOFeedbackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Fire an event. Unknown events and missing assets are silent, by design. */
	void Play(FGameplayTag Event, const FEOFeedbackContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Feedback", meta = (DisplayName = "Play Feedback At Location"))
	void PlayAtLocation(FGameplayTag Event, const FVector& Location, float Scale = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Feedback", meta = (DisplayName = "Play Feedback At Actor"))
	void PlayAtActor(FGameplayTag Event, AActor* Actor, float Scale = 1.f);

	/** Degrees to add to the current camera FOV. Decays to zero on its own. */
	UFUNCTION(BlueprintPure, Category = "Feedback")
	float GetFOVImpulse() const { return FOVImpulse; }

	/** 0..1 vignette the HUD draws. Already scaled by the accessibility setting. */
	UFUNCTION(BlueprintPure, Category = "Feedback")
	float GetVignetteAlpha() const { return VignetteAlpha; }

	/**
	 * Where the last damage came from, and how fresh it is.
	 * Returns false once the indicator has faded.
	 */
	UFUNCTION(BlueprintPure, Category = "Feedback")
	bool GetDamageDirection(FVector& OutWorldDirection, float& OutAlpha) const;

	/** Clears transient screen state. Used by the reset command and the self-test. */
	UFUNCTION(BlueprintCallable, Category = "Feedback")
	void ClearScreenState();

	//~ UTickableWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;
	//~ End UTickableWorldSubsystem

	/** Convenience for gameplay code that only has a world. Null-safe. */
	static UEOFeedbackSubsystem* Get(const UObject* WorldContext);

private:
	const FEOFeedbackPreset* FindPreset(FGameplayTag Event);

	/** Kicks off the preset set load. Everything stays silent until it lands. */
	void BeginWarmingPresets();

	/** Asks the streamer for every asset the presets reference, in one request. */
	void WarmPresetAssets();

	/**
	 * Assets resolve in the background and are read back with Get(), which returns
	 * null until they land. An event that fires before its asset is resident is
	 * skipped, exactly as a missing asset pack already is - the alternative is a
	 * blocking disk load on the frame the player pulls the trigger.
	 */
	TSharedPtr<struct FStreamableHandle> PresetSetHandle;
	TSharedPtr<struct FStreamableHandle> PresetAssetsHandle;

	APlayerController* GetLocalController() const;

	void ApplyAudio(const FEOFeedbackPreset& Preset, const FEOFeedbackContext& Context, float Scale);
	void ApplyEffect(const FEOFeedbackPreset& Preset, const FEOFeedbackContext& Context, float Scale);
	void ApplyCamera(const FEOFeedbackPreset& Preset, const FEOFeedbackContext& Context, float Scale);
	void ApplyScreen(const FEOFeedbackPreset& Preset, const FEOFeedbackContext& Context, float Scale);
	void ApplyHaptics(const FEOFeedbackPreset& Preset);
	void ApplyDecal(const FEOFeedbackPreset& Preset, const FEOFeedbackContext& Context);
	void ApplyHitStop(const FEOFeedbackPreset& Preset, const FEOFeedbackContext& Context);

	/** Restores an actor's time dilation after a hit-stop. */
	void EndHitStop(TWeakObjectPtr<AActor> Actor);

	UPROPERTY(Transient)
	TObjectPtr<UEOFeedbackPresetSet> LoadedPresets = nullptr;

	bool bPresetLoadAttempted = false;

	// ---- Transient screen state -------------------------------------------------

	float FOVImpulse = 0.f;
	float FOVImpulseDecay = 0.3f;

	float VignetteAlpha = 0.f;
	float VignetteDecay = 0.6f;

	FVector DamageDirection = FVector::ZeroVector;
	float DamageIndicatorAlpha = 0.f;

	/** Seconds a damage indicator stays up. Long enough to turn toward it. */
	static constexpr float DamageIndicatorDecay = 1.6f;
};
