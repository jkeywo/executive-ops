#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EOHudSettings.generated.h"

class UEOHudWidget;

/**
 * Which widgets draw the interface.
 *
 * A settings object rather than a property on a Blueprint subclass of the HUD,
 * because the project already reaches for UDeveloperSettings for the feedback
 * presets and the input config, and because the HUD class is set in C++ by the
 * game mode - there is no Blueprint of it to hang a class on.
 *
 * Both are optional. Until a widget class is assigned the corresponding surface
 * keeps drawing on Canvas, which is what lets the widgets be authored one at a
 * time against a running game rather than all at once against a blank one.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Executive Ops - HUD"))
class EXECUTIVEOPS_API UEOHudSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** The viewport HUD. A Widget Blueprint whose parent class is EOHudWidget. */
	UPROPERTY(config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UEOHudWidget> ViewportWidget;

	/**
	 * The in-world cockpit panel. Usually the same class as the viewport, since
	 * both bind to the same view model; a separate one is for when the panel
	 * wants a different arrangement of the same values.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Widgets")
	TSoftClassPtr<UEOHudWidget> PanelWidget;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	static const UEOHudSettings& Get() { return *GetDefault<UEOHudSettings>(); }
};
