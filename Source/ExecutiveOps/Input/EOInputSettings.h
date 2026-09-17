#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EOInputSettings.generated.h"

/**
 * Look preferences, read at the moment input arrives.
 *
 * These lived on UEOInputConfig as EditAnywhere properties, and could never take
 * effect: the config is built with NewObject on every run, the mappings bake the
 * inversion in once behind a bBuilt guard, and there is no rebuild path. So the
 * object the editor let you change was thrown away before it was ever consulted,
 * and two shipped settings silently did nothing.
 *
 * Reading them per event instead of baking them into a modifier means changing
 * one applies immediately, with no context rebuild and no second source of truth.
 *
 * These are project settings, not per-player ones. See Docs/adr/0008 - a player
 * facing options screen wants Enhanced Input's own user settings, which persist
 * per user; that is a larger job and is not what was broken.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Executive Ops - Input"))
class EXECUTIVEOPS_API UEOInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Invert vertical mouse look, relative to the standard scheme. */
	UPROPERTY(config, EditAnywhere, Category = "Look")
	bool bInvertMouseY = false;

	/** Invert vertical stick look. Separate from the mouse, deliberately. */
	UPROPERTY(config, EditAnywhere, Category = "Look")
	bool bInvertStickY = false;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	static const UEOInputSettings& Get() { return *GetDefault<UEOInputSettings>(); }

	/** Multiplier to apply to a vertical look axis: 1, or -1 when inverted. */
	static float MouseSign() { return Get().bInvertMouseY ? -1.f : 1.f; }
	static float StickSign() { return Get().bInvertStickY ? -1.f : 1.f; }
};
