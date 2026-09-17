#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "EOHudScreenComponent.generated.h"

class AEODebugHUD;
class UCanvas;
class UCanvasRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * The HUD, on a gently curved panel sitting in the world.
 *
 * Rather than reauthoring every readout as a 3D widget, this redraws the
 * existing canvas HUD into a render target and maps that onto a curved mesh.
 * Every element the HUD already draws - the compass, the contract list, the
 * waypoint, the speed block - comes along unchanged.
 *
 * The panel is a component, so wherever it is attached is what it moves with.
 * Mounted in the cockpit it stays with the airframe, which means looking around
 * pans the view across a fixed display instead of dragging the display along.
 */
UCLASS(ClassGroup = (ExecutiveOps), meta = (BlueprintSpawnableComponent))
class EXECUTIVEOPS_API UEOHudScreenComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UEOHudScreenComponent(const FObjectInitializer& ObjectInitializer);

	/** Builds the curved geometry and the render target. Safe to call again. */
	UFUNCTION(BlueprintCallable, Category = "HUD Screen")
	void BuildScreen();

	/**
	 * Redraws the panel from the given HUD.
	 *
	 * Called once per frame from the HUD itself, which is the only place with a
	 * valid canvas and the full draw chain.
	 */
	void RedrawFrom(AEODebugHUD* SourceHud);

	UFUNCTION(BlueprintPure, Category = "HUD Screen")
	UCanvasRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

protected:
	virtual void OnRegister() override;

	/** Bound to the render target; runs the HUD's own draw chain into it. */
	UFUNCTION()
	void DrawToRenderTarget(UCanvas* TargetCanvas, int32 Width, int32 Height);

	/**
	 * Horizontal sweep of the panel, degrees. Gentle: this is a windscreen, not a
	 * dome. Wide enough to carry a readout laid out for a full screen - at 62 the
	 * HUD was squeezed into the middle third and the text went unreadable.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "1", ClampMax = "170"))
	float ArcDegrees = 84.f;

	/** Distance from the panel's origin to the glass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "1"))
	float Radius = 120.f;

	/**
	 * Height as a multiple of the height the render target's aspect asks for.
	 *
	 * The panel's height is derived, not authored: an independent number can
	 * disagree with the arc, and a 16:9 readout stretched onto a 16:12 panel
	 * distorts everything on it. At 1.35 the contract boxes ran clean off the top
	 * of the screen, which is what an authored height bought.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "0.1"))
	float HeightScale = 1.f;

	/** More segments is a smoother curve; the cost is trivial at this size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "2", ClampMax = "64"))
	int32 Segments = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "64"))
	int32 ResolutionX = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "64"))
	int32 ResolutionY = 1080;

	/** Assigned by the setup script; samples the render target unlit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen")
	TObjectPtr<UMaterialInterface> ScreenMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Screen", meta = (ClampMin = "0"))
	float Brightness = 2.2f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasRenderTarget2D> RenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ScreenMaterialInstance;

	/** Set only for the duration of one redraw, so the callback knows its source. */
	UPROPERTY(Transient)
	TObjectPtr<AEODebugHUD> PendingHud;

	bool bBuilt = false;
};
