#pragma once

#include "CoreMinimal.h"
#include "UI/EONavigationHUD.h"
#include "EOPlayerHUD.generated.h"

class AEOAircraftPawn;
class AEOExtractionZone;
class AEOGuardCharacter;
class AEOMissionSite;
class AEOOperativeCharacter;
class AEOPlayerController;
class UEOMissionSubsystem;
class UFont;

/**
 * Where the eight slots land this frame.
 *
 * The frame is authored at 1280x720 and scaled by height, with the left column
 * pinned to the left edge, the right column to the right, and everything else
 * centred - so the slots hold their relationship at any aspect ratio rather than
 * stretching.
 */
struct FEOHUDLayout
{
	float Scale = 1.f;

	float LeftX = 0.f;
	float RightX = 0.f;
	float ColumnWidth = 0.f;

	float TopY = 0.f;
	float FeedY = 0.f;
	float BottomY = 0.f;
	float BottomHeight = 0.f;

	float BearingX = 0.f;
	float BearingWidth = 0.f;

	float ChargeX = 0.f;
	float ChargeY = 0.f;
	float ChargeWidth = 0.f;

	float CentreX = 0.f;
	float CentreY = 0.f;

	/** Design pixels to screen pixels. */
	float S(float DesignPixels) const { return DesignPixels * Scale; }
};

/** Everything the eight slots read, gathered once so no slot re-queries the world. */
struct FEOHUDState
{
	AEOPlayerController* Controller = nullptr;
	APawn* Pawn = nullptr;

	/** The craft, whether the player is in it or it is waiting overhead. */
	AEOAircraftPawn* Aircraft = nullptr;
	AEOOperativeCharacter* Operative = nullptr;

	UEOMissionSubsystem* Mission = nullptr;
	AEOMissionSite* Site = nullptr;
	AEOExtractionZone* Extraction = nullptr;

	/** Guards that are not dead, nearest first. */
	TArray<AEOGuardCharacter*> Guards;

	/** True while the player is flying rather than on foot. */
	bool bFlying = false;
};

/**
 * The player HUD: one frame of eight slots, drawn for both the aircraft and the
 * operative so that deploying swaps the contents of a slot and never its place.
 *
 *   1 asset (top left)      2 bearing (top centre)   3 standing (top right)
 *   4 feed  (left)          5 focus   (centre)
 *   6 self  (bottom left)   7 charge  (bottom centre) 8 commit (bottom right)
 *
 * Canvas rather than UMG, for the same reason the rest of the UI is: the project
 * stays free of binary assets, and a greybox HUD that can be read in a diff is
 * worth more right now than one that has to be opened in the editor.
 *
 * Every readout here is backed by something the prototype actually simulates.
 * Slots the design reserves for systems that do not exist yet - cloak, trace,
 * Authority/Heat, cyberware, perception - are left empty rather than faked.
 */
UCLASS()
class EXECUTIVEOPS_API AEOPlayerHUD : public AEONavigationHUD
{
	GENERATED_BODY()

public:
	AEOPlayerHUD();

	virtual void DrawHUD() override;

	/** Hides the frame without hiding the waypoint or the map. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleFrame() { bFrameVisible = !bFrameVisible; }

protected:
	/** Collects the frame's inputs. False if there is nothing worth drawing. */
	bool GatherState(FEOHUDState& OutState);

	FEOHUDLayout BuildLayout() const;

	void DrawAssetSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawBearingSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawStandingSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawFeedSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawFocusSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawSelfSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawChargeSlot(const FEOHUDLayout& L, const FEOHUDState& S);
	void DrawCommitSlot(const FEOHUDLayout& L, const FEOHUDState& S);

	// ---- Drawing primitives ----------------------------------------------------

	/** Panel ground plus hairline border, with an optional accent edge on the left. */
	void DrawPanel(float X, float Y, float W, float H, bool bAccentEdge, float Scale);

	void DrawBox(float X, float Y, float W, float H, const FLinearColor& Colour, float Thickness);

	/** Every string in the frame goes through here: a dark drop behind the glyphs. */
	void DrawShadowedText(const FString& Text, const FLinearColor& Colour, float X, float Y,
		UFont* Font, float Scale);

	/** Small dim caption, always upper case. */
	void DrawCaption(const FString& Text, float X, float Y, float Scale,
		const FLinearColor& Colour);

	/** Caption on the left, value right-aligned to the same line. */
	void DrawCaptionRow(const FString& Caption, const FString& Value, float X, float Y,
		float Width, float Scale, const FLinearColor& ValueColour);

	void DrawBody(const FString& Text, float X, float Y, float Scale, const FLinearColor& Colour);
	void DrawHeadline(const FString& Text, float X, float Y, float Scale, const FLinearColor& Colour);

	void DrawRightText(const FString& Text, float RightX, float Y, UFont* Font, float Scale,
		const FLinearColor& Colour);

	void DrawCentredText(const FString& Text, float CentreX, float Y, UFont* Font, float Scale,
		const FLinearColor& Colour);

	/** Notched meter. Empty segments are outlined so the total always reads. */
	void DrawSegmentBar(float X, float Y, float W, float H, int32 Segments, float Fraction,
		const FLinearColor& Fill);

	/** Keycap chip, e.g. [F]. Returns its width. */
	float DrawKeyChip(const FString& Key, float X, float Y, float Scale, const FLinearColor& Colour);

	void DrawCircleOutline(float X, float Y, float Radius, const FLinearColor& Colour,
		float Thickness, int32 Segments = 24);

	void DrawDiamond(float X, float Y, float Radius, const FLinearColor& Colour, float Thickness);

	/**
	 * Arc around the reticle. Dashed for a contact that is unsure, solid for one
	 * that is not - shape as well as colour, because the ladder has to read
	 * without relying on hue.
	 */
	void DrawThreatArc(float CentreX, float CentreY, float Radius, float MidAngleDeg,
		float SpanDeg, const FLinearColor& Colour, float Thickness, bool bDashed);

	/** Chevron ramp: how far up the alert ladder the site has climbed. */
	void DrawChevrons(float X, float Y, float Size, int32 Lit, int32 Total,
		const FLinearColor& Colour);

	// ---- Shared helpers --------------------------------------------------------

	/** Metres from the aircraft to whatever is below it, or -1 with nothing under. */
	float MeasureAltitude(const AEOAircraftPawn& Craft) const;

	/** One line of standing text for the mission, whichever pawn is possessed. */
	FString GetDirectiveText(const FEOHUDState& S) const;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	bool bFrameVisible = true;

	/** Total degrees of heading the compass ribbon covers. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float CompassSpanDegrees = 90.f;

	/** Design-pixel radius of the threat arcs around the reticle. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float ThreatArcRadius = 92.f;

	/** Seconds between rescans for guards and the aircraft. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float ActorScanInterval = 1.f;

private:
	/** Rescanned on an interval rather than every frame: the cast list rarely changes. */
	void RefreshActorCache();

	TArray<TWeakObjectPtr<AEOGuardCharacter>> CachedGuards;
	TWeakObjectPtr<AEOAircraftPawn> CachedAircraft;
	TWeakObjectPtr<AEOExtractionZone> CachedExtraction;

	float LastActorScanTime = -BIG_NUMBER;
};
