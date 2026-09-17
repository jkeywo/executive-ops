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

	/** Toggled by the EOToggleDebugHUD console command. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleVisible() { bDebugVisible = !bDebugVisible; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDebugVisible = true;

protected:
	/** One line of the readout, advancing Y. Named to avoid shadowing AHUD::DrawLine. */
	void DrawTextLine(const FString& Text, float& Y, const FLinearColor& Color = FLinearColor::White);
};
