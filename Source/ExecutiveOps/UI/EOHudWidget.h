#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/EOHudViewModel.h"
#include "EOHudWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UWidget;

/**
 * The interface, as a widget that copies from a view model and does nothing else.
 *
 * The layout is a Widget Blueprint parented to this class; the logic is here.
 * Every slot below is BindWidgetOptional, so a Blueprint that has not placed a
 * slot yet still compiles and runs - the value simply has nowhere to go. That
 * is deliberate: it lets the interface be built one slot at a time against a
 * running game. Names have to match exactly; Docs/HUD-Widgets.md lists them.
 *
 * Nothing here reads the world. If a value is missing, it is missing from
 * FEOHudViewModel, and that is where to add it - never by reaching past it.
 */
UCLASS(Abstract)
class EXECUTIVEOPS_API UEOHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Copies the frame's values into whichever slots the Blueprint placed. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void Apply(const FEOHudViewModel& ViewModel);

	/**
	 * Called after Apply with the same values, for anything the Blueprint wants
	 * to do that a text or a bar cannot - the compass tape, the threat arcs, the
	 * projected flight reticle.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnApplied(const FEOHudViewModel& ViewModel);

	/** The last values applied, for Blueprint bindings that read rather than push. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	const FEOHudViewModel& GetViewModel() const { return Current; }

	/** Widgets map tone to colour; this is the one place the mapping lives. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	static FLinearColor ColourForTone(EEOHudTone Tone);

protected:
	/**
	 * A HUD is a readout, never a control. Whatever the layout is built from,
	 * none of it may take the click that gives the game viewport mouse capture
	 * - a full-screen Border left Visible is a game that ignores every key.
	 */
	virtual void NativeOnInitialized() override;

protected:
	// ---- 1  Asset -----------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AssetCaption;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AssetMode;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AssetHeadline;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> AssetFill;

	// ---- 2  Bearing ---------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HeadingText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DirectiveLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Directive;

	// ---- 3  Standing --------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SiteState;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CertaintyText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> CertaintyFill;

	// ---- 4  Feed ------------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FeedCaption;
	/** The Blueprint fills its own list from GetViewModel().Feed in OnApplied. */

	// ---- 5  Focus -----------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PromptKey;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Prompt;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PromptGroup;

	// ---- 6  Self ------------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SpeedText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AltitudeText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VerticalSpeedText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> IntegrityText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> IntegrityFill;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> FlightReadouts;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> GroundReadouts;

	// ---- 7  Charge ----------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ChargeGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> HoverFill;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ChargeHint;

	// ---- 8  Commit ----------------------------------------------------------------
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CommitCaption;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CommitKey;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CommitHeadline;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CommitDetail;

private:
	UPROPERTY(Transient)
	FEOHudViewModel Current;

	static void SetText(UTextBlock* Block, const FText& Text, EEOHudTone Tone);
	static void SetFill(UProgressBar* Bar, float Fraction, EEOHudTone Tone);
	static void SetShown(UWidget* Widget, bool bShown);
};
