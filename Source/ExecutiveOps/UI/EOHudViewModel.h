#pragma once

#include "CoreMinimal.h"
#include "Interfaces/EOMissionTypes.h"
#include "EOHudViewModel.generated.h"

struct FEOHUDState;
enum class EEOGuardState : uint8;

/**
 * Everything the interface shows, as plain values.
 *
 * The second seam in the HUD. The first, FEOHUDState, is "what is in the
 * world"; this is "what the player sees", reduced to strings, fractions and
 * tiers with nothing about pixels in it. Widgets copy from it and do nothing
 * else, which is what makes the reductions here testable without a viewport,
 * and what lets the viewport and the in-world panel show the same thing.
 *
 * Built once per frame from an FEOHUDState by Build(). The static helpers below
 * it take plain inputs on purpose: they are the part worth asserting on, and
 * none of them should need an actor to be exercised.
 */

/** How urgently a value should read. Widgets map this to colour. */
UENUM(BlueprintType)
enum class EEOHudTone : uint8
{
	Dim,
	Ink,
	Accent,
	Caution,
	Alarm
};

/** One line in the feed: a contract, a pickup, or a contact. */
USTRUCT(BlueprintType)
struct FEOHudFeedRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FText Tag;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float DistanceM = 0.f;

	/** 0..1 for contacts; unused for contracts. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float Fill = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	EEOHudTone Tone = EEOHudTone::Ink;
};

/** One guard, as the reticle sees it. */
USTRUCT(BlueprintType)
struct FEOHudThreat
{
	GENERATED_BODY()

	/** Degrees clockwise from the view direction, -180..180. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float RelativeBearing = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float Detection = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	EEOHudTone Tone = EEOHudTone::Ink;
};

/** What the commit slot is offering on foot. */
UENUM(BlueprintType)
enum class EEOHudExtractionStatus : uint8
{
	NoRoute,
	Sealed,
	PadOpen,
	Extract,
	Inbound
};

USTRUCT(BlueprintType)
struct FEOHudViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bFlying = false;

	// ---- 1  Asset (top left) ----------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Asset")
	FText AssetCaption;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Asset")
	FText AssetMode;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Asset")
	FText AssetHeadline;

	/** Thrust while flying; unused on foot. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Asset")
	float AssetFill = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Asset")
	EEOHudTone AssetTone = EEOHudTone::Ink;

	// ---- 2  Bearing (top centre) ------------------------------------------------

	/** 0..359, as displayed. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	int32 Heading = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	FText HeadingText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	bool bHasSiteBearing = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	float SiteRelativeBearing = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	bool bHasExtractionBearing = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	float ExtractionRelativeBearing = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	FText DirectiveLabel;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Bearing")
	FText Directive;

	// ---- 3  Standing (top right) ------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Standing")
	FText SiteState;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Standing")
	EEOHudTone SiteStateTone = EEOHudTone::Dim;

	/** Highest detection across live guards, 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Standing")
	float Certainty = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Standing")
	FText CertaintyText;

	// ---- 4  Feed (left) ---------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Feed")
	FText FeedCaption;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Feed")
	TArray<FEOHudFeedRow> Feed;

	// ---- 5  Focus (centre) ------------------------------------------------------

	/** World point ahead of the craft, for the widget to project. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Focus")
	bool bHasFlightReticle = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Focus")
	FVector FlightReticleWorld = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Focus")
	bool bAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Focus")
	TArray<FEOHudThreat> Threats;

	/** Empty when there is nothing to prompt. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Focus")
	FText PromptKey;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Focus")
	FText Prompt;

	// ---- 6  Self (bottom left) --------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Self")
	FText SpeedText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Self")
	FText AltitudeText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Self")
	FText VerticalSpeedText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Self")
	float Integrity = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Self")
	FText IntegrityText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Self")
	EEOHudTone IntegrityTone = EEOHudTone::Ink;

	// ---- 7  Charge (bottom centre) ----------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Charge")
	bool bShowCharge = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Charge")
	float HoverBlend = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Charge")
	FText ChargeHint;

	// ---- 8  Commit (bottom right) -----------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Commit")
	FText CommitCaption;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Commit")
	FText CommitKey;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Commit")
	FText CommitHeadline;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Commit")
	FText CommitDetail;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Commit")
	EEOHudTone CommitTone = EEOHudTone::Dim;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Commit")
	EEOHudExtractionStatus Extraction = EEOHudExtractionStatus::NoRoute;

	// ---- Screen feedback --------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Feedback")
	float Vignette = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Feedback")
	float DamageAlpha = 0.f;

	/** Degrees clockwise from the view direction, -180..180. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Feedback")
	float DamageRelativeBearing = 0.f;

	// ---- Building -----------------------------------------------------------------

	/** The whole frame's values from the world. */
	static FEOHudViewModel Build(const FEOHUDState& State);

	// The reductions, on plain inputs, so they can be asserted on without a world.

	static FText DirectiveFor(bool bHasMission, EEOMissionState MissionState, const FString& SiteName);

	/**
	 * The worst state among the guards, as a tier: 0 unaware, 1 suspicious,
	 * 2 searching, 3 alerted. A patrolling guard with any awareness counts as
	 * suspicious, so the readout moves the moment a guard starts to notice.
	 */
	static int32 StandingTierFor(EEOGuardState GuardState, float Detection);
	static FText StandingTextFor(int32 Tier, bool bAnyGuards);
	static EEOHudTone StandingToneFor(int32 Tier, bool bAnyGuards);

	static EEOHudTone ToneForGuard(EEOGuardState GuardState);
	static FText ContactTextFor(EEOGuardState GuardState);

	static FText StanceFor(bool bDead, bool bTakedown, bool bDeploying, const FString& TraversalName,
		bool bSliding, bool bSprinting, bool bCrouched, bool bAiming);

	static EEOHudExtractionStatus ExtractionStatusFor(bool bInbound, bool bHasRoute, bool bAvailable, bool bInZone);
};
