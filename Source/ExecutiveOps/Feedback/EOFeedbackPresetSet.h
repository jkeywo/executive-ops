#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Feedback/EOFeedbackTypes.h"
#include "EOFeedbackPresetSet.generated.h"

/**
 * The whole game's feedback vocabulary, as one editable asset.
 *
 * One asset rather than one per event, because the useful question during a
 * polish pass is never "what does the pistol do" - it is "is the pistol louder
 * than the takedown", and that is only answerable with everything on screen at
 * once.
 *
 * Built by Scripts/m8_build_feedback_presets.py from whichever asset packs are
 * present, so a clone without the packs still runs, silently.
 */
UCLASS(BlueprintType)
class EXECUTIVEOPS_API UEOFeedbackPresetSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Keyed by the tags in EOFeedbackEvents.h. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TMap<FGameplayTag, FEOFeedbackPreset> Presets;

	/** Null when the event has no preset, which is a normal state, not an error. */
	const FEOFeedbackPreset* Find(FGameplayTag Event) const { return Presets.Find(Event); }
};
