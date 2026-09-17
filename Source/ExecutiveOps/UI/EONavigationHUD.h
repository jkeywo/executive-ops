#pragma once

#include "CoreMinimal.h"
#include "UI/EODebugHUD.h"
#include "EONavigationHUD.generated.h"

class AEOMissionSite;

/**
 * Flight navigation drawn on the canvas: a waypoint over the selected mission
 * site, an off-screen arrow pointing to it, and a top-down tactical map of the
 * district.
 *
 * Canvas rather than UMG on purpose. This is greybox navigation whose job is to
 * answer "can the player find the site and fly to it", and every part of it is
 * expected to be replaced once the real HUD exists.
 */
UCLASS()
class EXECUTIVEOPS_API AEONavigationHUD : public AEODebugHUD
{
	GENERATED_BODY()

public:
	AEONavigationHUD();

	virtual void DrawHUD() override;

	/** Toggled by the EOToggleMap console command. */
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void ToggleMap() { bMapVisible = !bMapVisible; }

protected:
	/** Waypoint diamond and distance over the site, or an arrow if off-screen. */
	void DrawWaypoint(const AEOMissionSite& Site, const FVector& ViewerLocation);

	/** Top-down plan of the district: buildings, the player, the site. */
	void DrawTacticalMap(const AEOMissionSite* Site, const APawn& Viewer);

	/** Collects the static geometry to plot. Cached: the greybox does not move. */
	void CacheMapGeometry();

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	bool bMapVisible = true;

	/** World centimetres covered by the tactical map's half-width. */
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float MapWorldExtent = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float MapScreenSize = 260.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float MapScreenMargin = 24.f;

	/**
	 * Extra distance down from the top margin. The player HUD owns the top right
	 * corner, so it pushes the map below its standing panel rather than under it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float MapTopOffset = 0.f;

	/** Half-height below which a mesh is treated as ground or overhead, not a building. */
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float MinPlottedHeight = 250.f;

	/** Seconds between geometry rescans, so streamed-in blocks eventually appear. */
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float GeometryRefreshInterval = 5.f;

private:
	/** Cached footprints: world XY centre and world XY half-extent. */
	struct FMapFootprint
	{
		FVector2D Centre = FVector2D::ZeroVector;
		FVector2D HalfExtent = FVector2D::ZeroVector;
	};

	TArray<FMapFootprint> MapGeometry;
	bool bGeometryCached = false;
	float LastCacheTime = 0.f;
};
