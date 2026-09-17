#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EODebugHUD.generated.h"

/**
 * Canvas-drawn debug readout: control mode, mission state, speed, deployment readiness.
 *
 * Deliberately not UMG — M0 stays free of binary assets, and this is scaffolding that
 * a real HUD will replace rather than build on.
 */
UCLASS()
class EXECUTIVEOPS_API AEODebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEODebugHUD();

	virtual void DrawHUD() override;

	/**
	 * Draws the screen flash after the whole DrawHUD chain has run.
	 *
	 * A base class draws first, so a flash drawn in DrawHUD would sit underneath
	 * the readouts of every subclass. PostRender invokes DrawHUD, so painting
	 * here - after Super - puts the wash over the lot, which is the point of it.
	 */
	virtual void PostRender() override;

	/**
	 * The panel currently showing this HUD, if any.
	 *
	 * When set, the whole draw chain is redirected onto it instead of the
	 * viewport, so the readouts live in the world rather than pasted over it.
	 * The screen-space flash still draws over the top - it is hiding a cut, and
	 * a cut is not something a diegetic panel can cover.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetProjectionScreen(class UEOHudScreenComponent* Screen);

	/**
	 * Runs the whole draw chain into the given canvas instead of the viewport.
	 *
	 * Every readout draws through AHUD::Canvas, so pointing that at a render
	 * target's canvas redirects all of them without any of them knowing. The
	 * member is protected, which is why this lives here rather than on the screen.
	 */
	void DrawInto(class UCanvas* TargetCanvas);

	/** Toggled by the EOToggleDebugHUD console command. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleVisible() { bDebugVisible = !bDebugVisible; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDebugVisible = true;

	UPROPERTY(Transient)
	TObjectPtr<class UEOHudScreenComponent> ProjectionScreen;

protected:
	/** One line of the readout, advancing Y. Named to avoid shadowing AHUD::DrawLine. */
	void DrawTextLine(const FString& Text, float& Y, const FLinearColor& Color = FLinearColor::White);
};
