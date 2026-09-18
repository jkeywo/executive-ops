#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "EOHudPanelComponent.generated.h"

class AEOPlayerHUD;
class UEOHudWidget;
struct FEOHudViewModel;

/**
 * The HUD, on a curved panel in the world - the second adapter over the view
 * model, and the reason the view model exists.
 *
 * A UWidgetComponent in Cylinder mode. The engine already draws a widget onto a
 * curved surface; the procedural mesh and render-target trick this replaces was
 * reimplementing that, and redirecting the entire Canvas draw chain into it to
 * get the readouts across. Now the same FEOHudViewModel the viewport widget
 * copies from is pushed here too, so the two show the same thing by
 * construction rather than by redrawing.
 *
 * Subscribes to the HUD rather than the HUD knowing about panels. The HUD
 * broadcasts each frame's view model; anything that shows the interface listens.
 * Nothing about the aircraft, or about first person, lives in the HUD.
 *
 * Optional, like the viewport widget: with no Panel Widget class assigned in
 * the HUD settings there is no widget and the component draws nothing, and the
 * old panel stays in use. Assigning one is the switch.
 */
UCLASS(ClassGroup = (ExecutiveOps), meta = (BlueprintSpawnableComponent))
class EXECUTIVEOPS_API UEOHudPanelComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UEOHudPanelComponent();

	/** True once a widget class is assigned and the widget exists. */
	UFUNCTION(BlueprintPure, Category = "HUD Panel")
	bool IsLive() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** The HUD may not exist yet at BeginPlay; tries again each tick until it does. */
	void TryBind();

	void HandleViewModel(const FEOHudViewModel& ViewModel);

	TWeakObjectPtr<AEOPlayerHUD> BoundHud;
	FDelegateHandle Handle;
};
