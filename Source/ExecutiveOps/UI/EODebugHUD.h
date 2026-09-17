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

	/** Toggled by the EOToggleDebugHUD console command. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleVisible() { bDebugVisible = !bDebugVisible; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDebugVisible = true;

private:
	void DrawLine(const FString& Text, float& Y, const FLinearColor& Color = FLinearColor::White);
};
