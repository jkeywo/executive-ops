#include "UI/EOPlayerHUD.h"

#include "UI/EOHudSettings.h"
#include "UI/EOHudStateGatherer.h"
#include "UI/EOHudViewModel.h"
#include "UI/EOHudWidget.h"

#include "Blueprint/UserWidget.h"
#include "UI/EOHUDUnits.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Character/EOTraversalComponent.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Core/EOPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EOExtractionInterface.h"
#include "Mission/EOExtractionZone.h"
#include "Mission/EOInteractableInterface.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOMissionSubsystem.h"

namespace
{
	// One ink, one dim, one accent, and two escalation colours. Escalation is the
	// only thing allowed to use hue, and it always carries a shape change as well.
	const FLinearColor Ink(0.902f, 0.945f, 0.957f);
	const FLinearColor Dim(0.576f, 0.655f, 0.686f);
	const FLinearColor Accent(0.275f, 0.847f, 0.745f);
	const FLinearColor Caution(0.949f, 0.663f, 0.231f);
	const FLinearColor Alarm(1.f, 0.361f, 0.431f);

	const FLinearColor PanelGround(0.020f, 0.031f, 0.039f, 0.74f);
	const FLinearColor PanelBorder(0.902f, 0.945f, 0.957f, 0.16f);
	const FLinearColor Hairline(0.902f, 0.945f, 0.957f, 0.10f);


	/** Dropped behind every glyph: the HUD has to read over a white rooftop as well as a night sky. */
	const FLinearColor TextShadow(0.f, 0.f, 0.f, 0.7f);

	UFont* SmallFont() { return GEngine ? GEngine->GetSmallFont() : nullptr; }
	UFont* MediumFont() { return GEngine ? GEngine->GetMediumFont() : nullptr; }
}

AEOPlayerHUD::AEOPlayerHUD()
{
	PrimaryActorTick.bCanEverTick = false;

	// The text readout is scaffolding, not the HUD. It stays one console command
	// away (EOToggleDebugHUD) rather than sitting on top of the frame.
	bDebugVisible = false;
}

void AEOPlayerHUD::BeginPlay()
{
	Super::BeginPlay();

	// Optional by design. No class assigned means no widget, and the Canvas slots
	// keep drawing - so the interface can be built one slot at a time against a
	// game that still shows everything.
	if (UClass* WidgetClass = UEOHudSettings::Get().ViewportWidget.LoadSynchronous())
	{
		Widget = CreateWidget<UEOHudWidget>(PlayerOwner, WidgetClass, TEXT("HudWidget"));
		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
}

void AEOPlayerHUD::DrawHUD()
{
	if (!Canvas)
	{
		Super::DrawHUD();
		return;
	}

	const FEOHUDLayout Layout = BuildLayout();

	// Push the inherited tactical map below the standing panel, which owns the top
	// right corner. Set here rather than once in the constructor because the panel
	// scales with the viewport and a fixed offset stops clearing it at 1440p and up.
	MapTopOffset = Layout.TopY + Layout.S(100.f);

	// Waypoint and tactical map first: the frame draws over them.
	Super::DrawHUD();

	if (!bFrameVisible)
	{
		return;
	}

	if (!Gatherer)
	{
		Gatherer = NewObject<UEOHudStateGatherer>(this, TEXT("HudStateGatherer"));
	}

	// Read the world once. Every slot below takes this and nothing else, which is
	// what stops nine readouts being derived twice per frame.
	FEOHUDState State;
	if (!Gatherer->Gather(Cast<AEOPlayerController>(PlayerOwner), State))
	{
		return;
	}

	// Built once, whoever is showing it. The cockpit panel listens for this
	// whether or not there is a viewport widget yet.
	const FEOHudViewModel ViewModel = FEOHudViewModel::Build(State);
	OnViewModelBuilt.Broadcast(ViewModel);

	// A live widget is the interface. The Canvas slots stand down rather than
	// drawing underneath it; the screen feedback stays, because it is hiding a
	// cut and a widget is not the thing to trust with that.
	if (Widget)
	{
		Widget->Apply(ViewModel);
		DrawScreenFeedback(Layout, State);
		return;
	}

	DrawAssetSlot(Layout, State);
	DrawBearingSlot(Layout, State);
	DrawStandingSlot(Layout, State);
	DrawFeedSlot(Layout, State);
	DrawFocusSlot(Layout, State);
	DrawSelfSlot(Layout, State);
	DrawChargeSlot(Layout, State);
	DrawCommitSlot(Layout, State);

	// Last, over everything: damage feedback that the frame could hide is damage
	// feedback that does not work.
	DrawScreenFeedback(Layout, State);
}

void AEOPlayerHUD::DrawScreenFeedback(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(this);
	if (!Feedback || !Canvas)
	{
		return;
	}

	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;

	const float Vignette = Feedback->GetVignetteAlpha();
	if (Vignette > 0.f)
	{
		// Built from four edge bands rather than a full-screen overlay: the brief
		// rules out full-screen flashes and sustained blur, and the centre of the
		// screen is exactly where the player needs to keep seeing.
		const float Depth = FMath::Min(W, H) * 0.14f;
		const int32 Steps = 6;

		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const float Band = Depth / Steps;
			const float Offset = Band * Step;

			// Densest at the edge, fading inward.
			const float Alpha = Vignette * 0.32f * (1.f - static_cast<float>(Step) / Steps);
			const FLinearColor Colour = Alarm.CopyWithNewOpacity(Alpha);

			DrawRect(Colour, 0.f, Offset, W, Band);					// top
			DrawRect(Colour, 0.f, H - Offset - Band, W, Band);		// bottom
			DrawRect(Colour, Offset, 0.f, Band, H);					// left
			DrawRect(Colour, W - Offset - Band, 0.f, Band, H);		// right
		}
	}

	FVector Direction;
	float DamageAlpha = 0.f;
	if (!Feedback->GetDamageDirection(Direction, DamageAlpha) || DamageAlpha <= 0.f)
	{
		return;
	}

	const APlayerController* PC = S.Controller ? S.Controller : PlayerOwner;
	if (!PC)
	{
		return;
	}

	// Screen-space bearing to the shooter, so the marker sits where the player
	// would have to turn, not where the damage numerically came from.
	const float ReferenceYaw = PC->GetControlRotation().Yaw;
	const float Bearing = FRotator::NormalizeAxis(Direction.Rotation().Yaw - ReferenceYaw);
	const float Radians = FMath::DegreesToRadians(Bearing);

	const float CX = W * 0.5f;
	const float CY = H * 0.5f;
	const float Radius = FMath::Min(W, H) * 0.24f;

	const float MarkerX = CX + FMath::Sin(Radians) * Radius;
	const float MarkerY = CY - FMath::Cos(Radians) * Radius;

	// An arc, not an arrow: it reads at a glance and matches the threat arc the
	// bearing slot already uses, so the player learns one shape rather than two.
	DrawThreatArc(CX, CY, Radius, Bearing, 26.f,
		Alarm.CopyWithNewOpacity(DamageAlpha), FMath::Max(2.f, L.Scale * 3.f),
		/*bDashed=*/false);

	DrawDiamond(MarkerX, MarkerY, L.S(7.f), Alarm.CopyWithNewOpacity(DamageAlpha),
		FMath::Max(1.f, L.Scale * 2.f));
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

FEOHUDLayout AEOPlayerHUD::BuildLayout() const
{
	FEOHUDLayout L;

	// Authored at 720p and scaled by height, so the frame keeps its proportions
	// and the left and right columns stay on their own edges at any aspect.
	L.Scale = FMath::Max(Canvas->SizeY / 720.f, 0.5f);

	L.ColumnWidth = L.S(320.f);
	L.LeftX = L.S(40.f);
	L.RightX = Canvas->SizeX - L.S(40.f) - L.ColumnWidth;

	L.TopY = L.S(34.f);
	L.FeedY = L.S(236.f);
	L.BottomHeight = L.S(124.f);
	L.BottomY = Canvas->SizeY - L.S(40.f) - L.BottomHeight;

	L.CentreX = Canvas->SizeX * 0.5f;
	L.CentreY = Canvas->SizeY * 0.5f;

	L.BearingWidth = L.S(480.f);
	L.BearingX = L.CentreX - L.BearingWidth * 0.5f;

	L.ChargeWidth = L.S(360.f);
	L.ChargeX = L.CentreX - L.ChargeWidth * 0.5f;
	L.ChargeY = Canvas->SizeY - L.S(120.f);

	return L;
}

// ---------------------------------------------------------------------------
// 1 - Asset. The craft, wherever the player happens to be.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawAssetSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float X = L.LeftX;
	const float Y = L.TopY;
	const float W = L.ColumnWidth;
	const float Pad = L.S(13.f);

	if (S.bFlying && S.Aircraft)
	{
		const float H = L.S(112.f);
		DrawPanel(X, Y, W, H, true, L.Scale);

		const bool bHover = IEOAircraftControlInterface::Execute_IsHovering(S.Aircraft);

		DrawCaptionRow(TEXT("AIRFRAME"), bHover ? TEXT("HOVER") : TEXT("FLIGHT"),
			X + Pad, Y + Pad, W - Pad * 2.f, L.Scale, bHover ? Accent : Dim);

		DrawHeadline(bHover ? TEXT("STATION KEEPING") : TEXT("FORWARD FLIGHT"),
			X + Pad, Y + L.S(36.f), L.Scale, Ink);

		DrawCaptionRow(TEXT("THRUST"),
			FString::Printf(TEXT("%.0f%%"), S.Aircraft->GetThrustAlpha() * 100.f),
			X + Pad, Y + L.S(68.f), W - Pad * 2.f, L.Scale, Ink);

		DrawSegmentBar(X + Pad, Y + L.S(90.f), W - Pad * 2.f, L.S(8.f), 10,
			S.Aircraft->GetThrustAlpha(), Accent);
		return;
	}

	// On the ground the same box answers the same question: where is the craft.
	const float H = L.S(82.f);
	DrawPanel(X, Y, W, H, true, L.Scale);

	const bool bInbound = S.Controller->IsExtractionInbound();

	DrawCaptionRow(TEXT("OVERWATCH"), bInbound ? TEXT("INBOUND") : TEXT("STANDBY"),
		X + Pad, Y + Pad, W - Pad * 2.f, L.Scale, bInbound ? Accent : Dim);

	FString Distance(TEXT("--"));
	if (bInbound)
	{
		Distance = FString::Printf(TEXT("%.0f M"), S.Controller->GetExtractionDistance());
	}
	else if (S.Aircraft)
	{
		Distance = FString::Printf(TEXT("%.0f M"),
			FVector::Dist(S.Aircraft->GetActorLocation(), S.Pawn->GetActorLocation()) * EOHUD::CmToM);
	}

	DrawHeadline(Distance, X + Pad, Y + L.S(36.f), L.Scale, Ink);
}

// ---------------------------------------------------------------------------
// 2 - Bearing. One compass ribbon, one standing directive under it.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawBearingSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float X = L.BearingX;
	const float W = L.BearingWidth;
	const float Y = L.TopY;
	const float RibbonH = L.S(40.f);
	const float Baseline = Y + RibbonH;
	const float HalfSpan = FMath::Max(CompassSpanDegrees * 0.5f, 5.f);

	const float ReferenceYaw = EOHUD::ReferenceYaw(*S.Pawn, *S.Controller, S.bFlying);

	auto BearingToX = [&](float AbsoluteBearing, bool& bVisible) -> float
	{
		const float Delta = FMath::FindDeltaAngleDegrees(ReferenceYaw, AbsoluteBearing);
		bVisible = FMath::Abs(Delta) <= HalfSpan;
		return L.CentreX + (Delta / HalfSpan) * (W * 0.5f);
	};

	DrawLine(X, Baseline, X + W, Baseline, PanelBorder.CopyWithNewOpacity(0.28f), FMath::Max(1.f, L.Scale));

	for (int32 Tick = 0; Tick < 360; Tick += 10)
	{
		bool bVisible = false;
		const float TickX = BearingToX(static_cast<float>(Tick), bVisible);
		if (!bVisible)
		{
			continue;
		}

		const bool bMajor = (Tick % 30) == 0;
		const float Height = bMajor ? L.S(14.f) : L.S(8.f);
		DrawLine(TickX, Baseline - Height, TickX, Baseline,
			Ink.CopyWithNewOpacity(bMajor ? 0.55f : 0.3f), FMath::Max(1.f, L.Scale));
	}

	// Cardinals, because a bare tick strip gives the player nothing to orient by.
	const TCHAR* const Cardinals[] = { TEXT("N"), TEXT("E"), TEXT("S"), TEXT("W") };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		bool bVisible = false;
		const float CardinalX = BearingToX(Index * 90.f, bVisible);
		if (bVisible)
		{
			DrawCentredText(Cardinals[Index], CardinalX, Y - L.S(2.f), SmallFont(), L.Scale, Dim);
		}
	}

	// Objective and extraction carry the same glyphs here as they do in the world:
	// filled diamond for the thing to reach, hollow for the way out.
	if (S.Site)
	{
		bool bVisible = false;
		const float MarkerX = BearingToX(
			ReferenceYaw + EOHUD::RelativeBearing(S.Pawn->GetActorLocation(), S.Site->GetHoverPoint(), ReferenceYaw),
			bVisible);
		if (bVisible)
		{
			DrawDiamond(MarkerX, Y + L.S(8.f), L.S(6.f), Accent, FMath::Max(1.f, L.Scale * 2.f));
		}
	}

	if (S.Extraction && IEOExtractionInterface::Execute_IsExtractionAvailable(S.Extraction))
	{
		bool bVisible = false;
		const float MarkerX = BearingToX(
			ReferenceYaw + EOHUD::RelativeBearing(S.Pawn->GetActorLocation(), S.Extraction->GetActorLocation(), ReferenceYaw),
			bVisible);
		if (bVisible)
		{
			DrawDiamond(MarkerX, Y + L.S(8.f), L.S(6.f), Ink, FMath::Max(1.f, L.Scale));
		}
	}

	// Heading caret and badge.
	DrawLine(L.CentreX - L.S(8.f), Y, L.CentreX - L.S(8.f), Baseline, Ink, FMath::Max(1.f, L.Scale));
	DrawLine(L.CentreX + L.S(8.f), Y, L.CentreX + L.S(8.f), Baseline, Ink, FMath::Max(1.f, L.Scale));

	const int32 Heading = FMath::RoundToInt(FRotator::ClampAxis(ReferenceYaw)) % 360;
	const FString HeadingText = FString::Printf(TEXT("%03d"), Heading);

	float BadgeW = 0.f;
	float BadgeH = 0.f;
	GetTextSize(HeadingText, BadgeW, BadgeH, SmallFont(), L.Scale * 1.2f);

	const float BadgeX = L.CentreX - BadgeW * 0.5f - L.S(6.f);
	DrawRect(Ink, BadgeX, Baseline, BadgeW + L.S(12.f), BadgeH + L.S(4.f));
	DrawText(HeadingText, FLinearColor(0.027f, 0.035f, 0.043f),
		BadgeX + L.S(6.f), Baseline + L.S(2.f), SmallFont(), L.Scale * 1.2f);

	// The standing directive: what the mission is asking for right now.
	const float DirectiveY = Baseline + BadgeH + L.S(14.f);
	DrawCentredText(S.bFlying ? TEXT("DIRECTIVE") : TEXT("OBJECTIVE"),
		L.CentreX, DirectiveY, SmallFont(), L.Scale, Dim);
	DrawCentredText(GetDirectiveText(S), L.CentreX, DirectiveY + L.S(13.f),
		SmallFont(), L.Scale * 1.2f, Ink);
}

FString AEOPlayerHUD::GetDirectiveText(const FEOHUDState& S) const
{
	if (!S.Mission)
	{
		return TEXT("NO MISSION CONTROL");
	}

	const FString SiteName = S.Site ? S.Site->GetDisplayName().ToString().ToUpper() : TEXT("UNKNOWN SITE");

	switch (S.Mission->GetMissionState())
	{
	case EEOMissionState::Inactive:			return TEXT("NO MISSION SELECTED");
	case EEOMissionState::InFlight:			return FString::Printf(TEXT("APPROACH %s"), *SiteName);
	case EEOMissionState::Deploying:		return TEXT("DEPLOYING");
	case EEOMissionState::OnGround:			return TEXT("SECURE THE OBJECTIVE");
	case EEOMissionState::ObjectiveComplete:	return TEXT("OBJECTIVE SECURE - EXTRACT");
	case EEOMissionState::Extracting:		return TEXT("EXTRACTION INBOUND");
	case EEOMissionState::Complete:			return TEXT("MISSION COMPLETE");
	case EEOMissionState::Failed:			return TEXT("OPERATIVE DOWN");
	default:					return TEXT("--");
	}
}

// ---------------------------------------------------------------------------
// 3 - Standing. How much the site knows, in the same box in both modes.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawStandingSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float X = L.RightX;
	const float Y = L.TopY;
	const float W = L.ColumnWidth;
	const float Pad = L.S(13.f);
	const float H = L.S(96.f);

	DrawPanel(X, Y, W, H, false, L.Scale);

	// The alert ladder the prototype actually has: one rung per guard state, with
	// the highest rung any guard has reached standing for the site.
	int32 Rung = 0;
	int32 Aware = 0;
	float MaxDetection = 0.f;

	for (const AEOGuardCharacter* Guard : S.Guards)
	{
		MaxDetection = FMath::Max(MaxDetection, Guard->GetDetectionAlpha());

		switch (Guard->GetGuardState())
		{
		case EEOGuardState::Alerted:	Rung = FMath::Max(Rung, 3); ++Aware; break;
		case EEOGuardState::Searching:	Rung = FMath::Max(Rung, 2); ++Aware; break;
		case EEOGuardState::Suspicious:	Rung = FMath::Max(Rung, 1); ++Aware; break;
		default:
			if (Guard->GetDetectionAlpha() > 0.f)
			{
				Rung = FMath::Max(Rung, 1);
				++Aware;
			}
			break;
		}
	}

	FString StateText = TEXT("NO CONTACT");
	FLinearColor StateColour = Dim;

	if (S.Guards.Num() > 0)
	{
		switch (Rung)
		{
		case 3:	StateText = TEXT("ALERTED");	StateColour = Alarm;	break;
		case 2:	StateText = TEXT("SEARCHING");	StateColour = Caution;	break;
		case 1:	StateText = TEXT("SUSPICIOUS");	StateColour = Ink;	break;
		default:	StateText = TEXT("UNAWARE");	StateColour = Dim;	break;
		}
	}

	DrawCaptionRow(TEXT("SITE STATE"), StateText, X + Pad, Y + Pad, W - Pad * 2.f, L.Scale, StateColour);

	DrawChevrons(X + Pad, Y + L.S(36.f), L.S(12.f), Rung, 3, StateColour);

	DrawCaptionRow(TEXT("CERTAINTY"), FString::Printf(TEXT("%.0f%%"), MaxDetection * 100.f),
		X + Pad, Y + L.S(56.f), W - Pad * 2.f, L.Scale, MaxDetection > 0.f ? Caution : Dim);

	DrawSegmentBar(X + Pad, Y + L.S(78.f), W - Pad * 2.f, L.S(7.f), 6, MaxDetection,
		Rung >= 3 ? Alarm : Caution);

	// Authority and Heat belong in this slot too, under the ladder. Neither
	// exists yet, so the space below stays empty rather than showing a stub.
	(void)Aware;
}

// ---------------------------------------------------------------------------
// 4 - Feed. What is out there, quiet by default.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawFeedSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float X = L.LeftX;
	const float W = L.ColumnWidth;
	const float RowHeight = L.S(40.f);
	float Y = L.FeedY;

	const FVector ViewerLocation = S.Pawn->GetActorLocation();

	if (S.bFlying)
	{
		struct FFeedRow
		{
			FString Title;
			FString Kind;
			float DistanceM = 0.f;
			bool bFilledGlyph = false;
		};

		TArray<FFeedRow> Rows;

		if (S.Site)
		{
			Rows.Add({ S.Site->GetDisplayName().ToString().ToUpper(), TEXT("CONTRACT"),
				static_cast<float>(FVector::Dist(ViewerLocation, S.Site->GetHoverPoint()) * EOHUD::CmToM), true });
		}

		if (S.Extraction && IEOExtractionInterface::Execute_IsExtractionAvailable(S.Extraction))
		{
			Rows.Add({ TEXT("EXTRACTION PAD"), TEXT("PICKUP"),
				static_cast<float>(FVector::Dist(ViewerLocation, S.Extraction->GetActorLocation()) * EOHUD::CmToM), false });
		}

		if (Rows.Num() == 0)
		{
			return;
		}

		DrawCaption(FString::Printf(TEXT("SCAN - %d RETURN%s"), Rows.Num(),
			Rows.Num() == 1 ? TEXT("") : TEXT("S")), X, Y, L.Scale, Dim);
		Y += L.S(20.f);
		DrawLine(X, Y, X + W, Y, PanelBorder, FMath::Max(1.f, L.Scale));

		for (const FFeedRow& Row : Rows)
		{
			const float GlyphY = Y + RowHeight * 0.5f;
			DrawDiamond(X + L.S(6.f), GlyphY, L.S(5.f), Row.bFilledGlyph ? Accent : Ink,
				FMath::Max(1.f, L.Scale * (Row.bFilledGlyph ? 2.f : 1.f)));

			DrawBody(Row.Title, X + L.S(20.f), Y + L.S(4.f), L.Scale, Ink);
			DrawCaption(Row.Kind, X + L.S(20.f), Y + L.S(24.f), L.Scale, Dim);
			DrawRightText(FString::Printf(TEXT("%.0fM"), Row.DistanceM), X + W, Y + L.S(6.f),
				SmallFont(), L.Scale, Dim);

			Y += RowHeight;
			DrawLine(X, Y, X + W, Y, Hairline, FMath::Max(1.f, L.Scale));
		}
		return;
	}

	// On foot the slot stays empty until somebody knows something. An empty
	// contact log is the readout: nobody has anything on you.
	TArray<const AEOGuardCharacter*> Contacts;
	for (const AEOGuardCharacter* Guard : S.Guards)
	{
		if (Guard->GetGuardState() != EEOGuardState::Patrolling || Guard->GetDetectionAlpha() > 0.f)
		{
			Contacts.Add(Guard);
		}
	}

	if (Contacts.Num() == 0)
	{
		return;
	}

	DrawCaption(TEXT("CONTACT LOG"), X, Y, L.Scale, Dim);
	Y += L.S(20.f);
	DrawLine(X, Y, X + W, Y, PanelBorder, FMath::Max(1.f, L.Scale));

	for (const AEOGuardCharacter* Guard : Contacts)
	{
		FString StateText = TEXT("SUSPICIOUS");
		FLinearColor Colour = Ink;

		switch (Guard->GetGuardState())
		{
		case EEOGuardState::Alerted:	StateText = TEXT("ALERTED");	Colour = Alarm;		break;
		case EEOGuardState::Searching:	StateText = TEXT("SEARCHING");	Colour = Caution;	break;
		case EEOGuardState::Suspicious:	StateText = TEXT("SUSPICIOUS");	Colour = Caution;	break;
		default:			StateText = TEXT("LOOKING");	Colour = Ink;		break;
		}

		DrawBody(StateText, X + L.S(6.f), Y + L.S(4.f), L.Scale, Colour);
		DrawRightText(FString::Printf(TEXT("%.0fM"),
			FVector::Dist(ViewerLocation, Guard->GetActorLocation()) * EOHUD::CmToM),
			X + W, Y + L.S(6.f), SmallFont(), L.Scale, Dim);

		DrawSegmentBar(X + L.S(6.f), Y + L.S(28.f), W - L.S(12.f), L.S(5.f), 5,
			Guard->GetDetectionAlpha(), Colour);

		Y += RowHeight;
		DrawLine(X, Y, X + W, Y, Hairline, FMath::Max(1.f, L.Scale));
	}
}

// ---------------------------------------------------------------------------
// 5 - Focus. Screen centre, in both modes.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawFocusSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float CX = L.CentreX;
	const float CY = L.CentreY;
	const float Thin = FMath::Max(1.f, L.Scale);

	if (S.bFlying && S.Aircraft)
	{
		// Horizon reference either side of the marker, so the eye has something
		// level to read the craft's attitude against.
		DrawLine(CX - L.S(180.f), CY, CX - L.S(70.f), CY, Ink.CopyWithNewOpacity(0.45f), Thin * 1.5f);
		DrawLine(CX + L.S(70.f), CY, CX + L.S(180.f), CY, Ink.CopyWithNewOpacity(0.45f), Thin * 1.5f);

		// Flight path marker: where the craft is actually going, which is not
		// where it is pointing once momentum is involved.
		float MarkerX = CX;
		float MarkerY = CY;

		const FVector Velocity = S.Aircraft->GetVelocity();
		if (Velocity.SizeSquared() > FMath::Square(150.f))
		{
			const FVector Ahead = S.Aircraft->GetActorLocation() + Velocity.GetSafeNormal() * 20000.f;
			const FVector Screen = Project(Ahead);
			if (Screen.Z > 0.f)
			{
				// Clamped, so a hard sideslip parks the marker at the edge of the
				// reticle instead of flying off the screen.
				const FVector2D Offset(Screen.X - CX, Screen.Y - CY);
				const FVector2D Clamped = Offset.SizeSquared() > FMath::Square(L.S(220.f))
					? Offset.GetSafeNormal() * L.S(220.f) : Offset;
				MarkerX = CX + Clamped.X;
				MarkerY = CY + Clamped.Y;
			}
		}

		const float R = L.S(13.f);
		DrawCircleOutline(MarkerX, MarkerY, R, Ink, Thin * 1.8f);
		DrawLine(MarkerX - R, MarkerY, MarkerX - R - L.S(17.f), MarkerY, Ink, Thin * 1.8f);
		DrawLine(MarkerX + R, MarkerY, MarkerX + R + L.S(17.f), MarkerY, Ink, Thin * 1.8f);
		DrawLine(MarkerX, MarkerY - R, MarkerX, MarkerY - R - L.S(9.f), Ink, Thin * 1.8f);
		return;
	}

	if (!S.Operative)
	{
		return;
	}

	// A dot and four ticks. Small on purpose: the ground screen should be mostly
	// world, and the arcs are what the player is actually reading.
	const bool bAiming = S.Operative->IsAiming();
	const float Inner = bAiming ? L.S(8.f) : L.S(16.f);
	const float Outer = Inner + L.S(8.f);

	DrawRect(Ink, CX - Thin, CY - Thin, Thin * 2.f, Thin * 2.f);
	DrawLine(CX - Outer, CY, CX - Inner, CY, Ink.CopyWithNewOpacity(0.6f), Thin);
	DrawLine(CX + Inner, CY, CX + Outer, CY, Ink.CopyWithNewOpacity(0.6f), Thin);
	DrawLine(CX, CY - Outer, CX, CY - Inner, Ink.CopyWithNewOpacity(0.6f), Thin);
	DrawLine(CX, CY + Inner, CX, CY + Outer, Ink.CopyWithNewOpacity(0.6f), Thin);

	// Threat arcs: direction and certainty, never positions. Dashed while a guard
	// is still making its mind up, solid once it has.
	const float ReferenceYaw = EOHUD::ReferenceYaw(*S.Pawn, *S.Controller, S.bFlying);
	const float Radius = L.S(ThreatArcRadius);

	for (const AEOGuardCharacter* Guard : S.Guards)
	{
		const bool bAlerted = Guard->GetGuardState() == EEOGuardState::Alerted;
		const bool bSearching = Guard->GetGuardState() == EEOGuardState::Searching;
		const float Detection = Guard->GetDetectionAlpha();

		if (!bAlerted && !bSearching && Detection <= 0.f)
		{
			continue;
		}

		const float Bearing = EOHUD::RelativeBearing(S.Pawn->GetActorLocation(), Guard->GetActorLocation(), ReferenceYaw);

		const FLinearColor Colour = bAlerted ? Alarm : (bSearching ? Caution : Ink);
		const float Thickness = bAlerted ? Thin * 5.f : Thin * (2.f + Detection * 2.f);
		const float Span = bAlerted ? 34.f : 22.f;

		DrawThreatArc(CX, CY, Radius, Bearing, Span, Colour, Thickness, !bAlerted && Detection < 1.f);
	}

	// The one verb on offer, under the reticle where the eye already is.
	if (const AEOGuardCharacter* Takedown = S.Operative->FindTakedownTarget())
	{
		const float PromptY = CY + L.S(56.f);
		const float ChipW = DrawKeyChip(TEXT("F"), CX - L.S(52.f), PromptY, L.Scale, Accent);
		DrawBody(TEXT("TAKEDOWN"), CX - L.S(52.f) + ChipW + L.S(8.f), PromptY + L.S(2.f), L.Scale, Accent);
	}
	else if (AActor* Interactable = S.Operative->FindInteractable())
	{
		DrawCentredText(IEOInteractableInterface::Execute_GetInteractionPrompt(Interactable).ToString(),
			CX, CY + L.S(56.f), SmallFont(), L.Scale * 1.2f, Caution);
	}
}

// ---------------------------------------------------------------------------
// 6 - Self. What shape the player is in.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawSelfSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float X = L.LeftX;
	const float Y = L.BottomY;
	const float W = L.ColumnWidth;
	const float H = L.BottomHeight;
	const float Pad = L.S(13.f);

	DrawPanel(X, Y, W, H, false, L.Scale);

	if (S.bFlying && S.Aircraft)
	{
		// Three numerals, because that is what flying one of these needs: how fast,
		// how high, and which way the altitude is going.
		const float Speed = IEOAircraftControlInterface::Execute_GetCurrentSpeed(S.Aircraft) * EOHUD::CmsToKph;
		const float Altitude = MeasureAltitude(*S.Aircraft);
		const float Vertical = S.Aircraft->GetVelocity().Z * EOHUD::CmToM;

		const float ColumnW = (W - Pad * 2.f) / 3.f;

		DrawCaption(TEXT("SPEED KM/H"), X + Pad, Y + Pad, L.Scale, Dim);
		DrawHeadline(FString::Printf(TEXT("%.0f"), Speed), X + Pad, Y + Pad + L.S(20.f), L.Scale, Ink);

		DrawCaption(TEXT("ALT M"), X + Pad + ColumnW, Y + Pad, L.Scale, Dim);
		DrawHeadline(Altitude < 0.f ? TEXT("--") : FString::Printf(TEXT("%.0f"), Altitude),
			X + Pad + ColumnW, Y + Pad + L.S(20.f), L.Scale, Ink);

		DrawCaption(TEXT("V/S M/S"), X + Pad + ColumnW * 2.f, Y + Pad, L.Scale, Dim);
		DrawHeadline(FString::Printf(TEXT("%+.0f"), Vertical),
			X + Pad + ColumnW * 2.f, Y + Pad + L.S(20.f), L.Scale,
			FMath::Abs(Vertical) < 1.f ? Ink : Accent);

		// Hull integrity would sit on the row below. The craft has no damage model
		// yet, so the row is left out rather than shown permanently full.
		return;
	}

	if (!S.Operative)
	{
		return;
	}

	const UEOHealthComponent* Health = S.Operative->GetHealth();
	const float Fraction = Health ? Health->GetHealthFraction() : 1.f;

	// Stance, by priority: whatever the body is committed to outranks what it
	// would otherwise be doing.
	FString Stance = TEXT("STANDING");
	if (S.Operative->IsDead())					{ Stance = TEXT("DOWN"); }
	else if (S.Operative->IsPerformingTakedown())			{ Stance = TEXT("TAKEDOWN"); }
	else if (S.Operative->IsDeploying())				{ Stance = TEXT("DROPPING"); }
	else if (const UEOTraversalComponent* Traversal = S.Operative->GetTraversal())
	{
		if (Traversal->IsTraversing())
		{
			Stance = StaticEnum<EEOTraversalType>()
				->GetNameStringByValue(static_cast<int64>(Traversal->GetActiveType())).ToUpper();
		}
	}

	if (Stance == TEXT("STANDING"))
	{
		if (S.Operative->IsSliding())		{ Stance = TEXT("SLIDING"); }
		else if (S.Operative->IsSprinting())	{ Stance = TEXT("SPRINTING"); }
		else if (S.Operative->bIsCrouched)	{ Stance = TEXT("CROUCHED"); }
		else if (S.Operative->IsAiming())	{ Stance = TEXT("AIMING"); }
	}

	DrawCaptionRow(TEXT("INTEGRITY"),
		FString::Printf(TEXT("%.0f%% - %s"), Fraction * 100.f, *Stance),
		X + Pad, Y + Pad, W - Pad * 2.f, L.Scale,
		Fraction < 0.4f ? Alarm : Ink);

	// A plain bar rather than segments: this is the one readout that has to be
	// legible out of the corner of the eye mid-sprint.
	const float BarY = Y + Pad + L.S(24.f);
	const float BarW = W - Pad * 2.f;
	DrawRect(Ink.CopyWithNewOpacity(0.14f), X + Pad, BarY, BarW, L.S(6.f));
	DrawRect(Fraction < 0.4f ? Alarm : Ink, X + Pad, BarY, BarW * FMath::Clamp(Fraction, 0.f, 1.f), L.S(6.f));

	// Cyberware slots belong under here. There is no cyberware yet.
}

// ---------------------------------------------------------------------------
// 7 - Charge. The capability that is filling or draining.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawChargeSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	// On the ground this slot is perception's (M9), and nothing else may take it -
	// the whole point of the slot is that the player learns one place to look for
	// "the thing that is charging". Empty until then.
	if (!S.bFlying || !S.Aircraft)
	{
		return;
	}

	const float Blend = S.Aircraft->GetHoverBlend();

	DrawCaption(TEXT("HOVER"), L.ChargeX, L.ChargeY, L.Scale, Blend > 0.f ? Accent : Dim);
	DrawRightText(TEXT("SPACE TOGGLES"), L.ChargeX + L.ChargeWidth, L.ChargeY,
		SmallFont(), L.Scale, Dim);

	DrawSegmentBar(L.ChargeX, L.ChargeY + L.S(22.f), L.ChargeWidth, L.S(10.f), 10, Blend, Accent);
}

// ---------------------------------------------------------------------------
// 8 - Commit. The one action worth committing to from here.
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawCommitSlot(const FEOHUDLayout& L, const FEOHUDState& S)
{
	const float X = L.RightX;
	const float Y = L.BottomY;
	const float W = L.ColumnWidth;
	const float H = L.BottomHeight;
	const float Pad = L.S(13.f);

	DrawPanel(X, Y, W, H, false, L.Scale);

	if (S.bFlying)
	{
		// Prompt and gate come from the same test, so the HUD can never offer a
		// drop the key would then refuse.
		const FString Blocker = S.Controller->GetDeploymentBlocker();
		const bool bReady = Blocker.IsEmpty();
		const FLinearColor Colour = bReady ? Accent : Dim;

		DrawCaption(TEXT("INSERTION"), X + Pad, Y + Pad, L.Scale, Dim);

		const float ChipW = DrawKeyChip(TEXT("F"), X + Pad, Y + Pad + L.S(16.f), L.Scale, Colour);
		DrawHeadline(TEXT("DEPLOY"), X + Pad + ChipW + L.S(10.f), Y + Pad + L.S(14.f), L.Scale, Colour);

		if (!bReady)
		{
			DrawCaption(Blocker.ToUpper(), X + Pad, Y + Pad + L.S(44.f), L.Scale, Dim);
		}

		const float MapY = Y + H - Pad - L.S(14.f);
		DrawLine(X + Pad, MapY - L.S(8.f), X + W - Pad, MapY - L.S(8.f), Hairline, FMath::Max(1.f, L.Scale));

		const float MapChipW = DrawKeyChip(TEXT("M"), X + Pad, MapY, L.Scale, Dim);
		DrawCaption(TEXT("TACTICAL MAP"), X + Pad + MapChipW + L.S(10.f), MapY + L.S(3.f), L.Scale, Dim);
		return;
	}

	DrawCaption(TEXT("EXTRACTION"), X + Pad, Y + Pad, L.Scale, Dim);

	if (S.Controller->IsExtractionInbound())
	{
		DrawHeadline(TEXT("INBOUND"), X + Pad, Y + Pad + L.S(14.f), L.Scale, Accent);
		DrawCaption(FString::Printf(TEXT("%.0f M TO PICKUP"), S.Controller->GetExtractionDistance()),
			X + Pad, Y + Pad + L.S(40.f), L.Scale, Dim);
		return;
	}

	if (!S.Extraction)
	{
		DrawHeadline(TEXT("NO ROUTE"), X + Pad, Y + Pad + L.S(14.f), L.Scale, Dim);
		return;
	}

	const bool bAvailable = IEOExtractionInterface::Execute_IsExtractionAvailable(S.Extraction);
	const float DistanceM = FVector::Dist(S.Extraction->GetActorLocation(), S.Pawn->GetActorLocation()) * EOHUD::CmToM;

	if (!bAvailable)
	{
		DrawHeadline(TEXT("SEALED"), X + Pad, Y + Pad + L.S(14.f), L.Scale, Dim);
		DrawCaption(TEXT("OBJECTIVE INCOMPLETE"), X + Pad, Y + Pad + L.S(40.f), L.Scale, Dim);
		return;
	}

	const bool bInZone = S.Extraction->IsWithinZone(S.Pawn);

	if (bInZone)
	{
		const float ChipW = DrawKeyChip(TEXT("E"), X + Pad, Y + Pad + L.S(16.f), L.Scale, Accent);
		DrawHeadline(TEXT("EXTRACT"), X + Pad + ChipW + L.S(10.f), Y + Pad + L.S(14.f), L.Scale, Accent);
	}
	else
	{
		DrawHeadline(TEXT("PAD OPEN"), X + Pad, Y + Pad + L.S(14.f), L.Scale, Accent);
		DrawCaption(FString::Printf(TEXT("%.0f M"), DistanceM), X + Pad, Y + Pad + L.S(40.f), L.Scale, Dim);
	}
}

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------

void AEOPlayerHUD::DrawPanel(float X, float Y, float W, float H, bool bAccentEdge, float Scale)
{
	DrawRect(PanelGround, X, Y, W, H);
	DrawBox(X, Y, W, H, PanelBorder, FMath::Max(1.f, Scale));

	if (bAccentEdge)
	{
		DrawRect(Accent, X, Y, FMath::Max(2.f * Scale, 1.f), H);
	}
}

void AEOPlayerHUD::DrawBox(float X, float Y, float W, float H, const FLinearColor& Colour, float Thickness)
{
	DrawLine(X, Y, X + W, Y, Colour, Thickness);
	DrawLine(X + W, Y, X + W, Y + H, Colour, Thickness);
	DrawLine(X + W, Y + H, X, Y + H, Colour, Thickness);
	DrawLine(X, Y + H, X, Y, Colour, Thickness);
}

void AEOPlayerHUD::DrawShadowedText(const FString& Text, const FLinearColor& Colour, float X, float Y,
	UFont* Font, float Scale)
{
	// Nothing in the frame is guaranteed a panel behind it, and unshadowed grey
	// text vanishes completely against the greybox rooftops.
	const float Offset = FMath::Max(1.f, Scale);
	DrawText(Text, TextShadow, X + Offset, Y + Offset, Font, Scale);
	DrawText(Text, Colour, X, Y, Font, Scale);
}

void AEOPlayerHUD::DrawCaption(const FString& Text, float X, float Y, float Scale, const FLinearColor& Colour)
{
	DrawShadowedText(Text.ToUpper(), Colour, X, Y, SmallFont(), Scale);
}

void AEOPlayerHUD::DrawCaptionRow(const FString& Caption, const FString& Value, float X, float Y,
	float Width, float Scale, const FLinearColor& ValueColour)
{
	DrawCaption(Caption, X, Y, Scale, Dim);
	DrawRightText(Value.ToUpper(), X + Width, Y, SmallFont(), Scale, ValueColour);
}

void AEOPlayerHUD::DrawBody(const FString& Text, float X, float Y, float Scale, const FLinearColor& Colour)
{
	DrawShadowedText(Text.ToUpper(), Colour, X, Y, SmallFont(), Scale * 1.2f);
}

void AEOPlayerHUD::DrawHeadline(const FString& Text, float X, float Y, float Scale, const FLinearColor& Colour)
{
	DrawShadowedText(Text.ToUpper(), Colour, X, Y, MediumFont(), Scale);
}

void AEOPlayerHUD::DrawRightText(const FString& Text, float RightX, float Y, UFont* Font, float Scale,
	const FLinearColor& Colour)
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Text, Width, Height, Font, Scale);
	DrawShadowedText(Text, Colour, RightX - Width, Y, Font, Scale);
}

void AEOPlayerHUD::DrawCentredText(const FString& Text, float CentreX, float Y, UFont* Font, float Scale,
	const FLinearColor& Colour)
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Text, Width, Height, Font, Scale);
	DrawShadowedText(Text, Colour, CentreX - Width * 0.5f, Y, Font, Scale);
}

void AEOPlayerHUD::DrawSegmentBar(float X, float Y, float W, float H, int32 Segments, float Fraction,
	const FLinearColor& Fill)
{
	Segments = FMath::Max(Segments, 1);
	Fraction = FMath::Clamp(Fraction, 0.f, 1.f);

	const float Gap = FMath::Max(W * 0.006f, 1.f);
	const float SegmentW = (W - Gap * (Segments - 1)) / Segments;

	// Part-filled segments are drawn faded rather than dropped, so a meter that is
	// moving reads as moving instead of stepping.
	const float Filled = Fraction * Segments;

	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float SegX = X + Index * (SegmentW + Gap);
		const float Remainder = Filled - Index;

		if (Remainder >= 1.f)
		{
			DrawRect(Fill, SegX, Y, SegmentW, H);
		}
		else if (Remainder > 0.f)
		{
			DrawRect(Fill.CopyWithNewOpacity(0.35f + Remainder * 0.4f), SegX, Y, SegmentW, H);
		}
		else
		{
			DrawBox(SegX, Y, SegmentW, H, PanelBorder.CopyWithNewOpacity(0.25f), 1.f);
		}
	}
}

float AEOPlayerHUD::DrawKeyChip(const FString& Key, float X, float Y, float Scale, const FLinearColor& Colour)
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Key, Width, Height, SmallFont(), Scale * 1.2f);

	const float PadX = 6.f * Scale;
	const float PadY = 3.f * Scale;
	const float BoxW = Width + PadX * 2.f;
	const float BoxH = Height + PadY * 2.f;

	DrawBox(X, Y, BoxW, BoxH, Colour, FMath::Max(1.f, Scale));
	DrawShadowedText(Key, Colour, X + PadX, Y + PadY, SmallFont(), Scale * 1.2f);

	return BoxW;
}

void AEOPlayerHUD::DrawCircleOutline(float X, float Y, float Radius, const FLinearColor& Colour,
	float Thickness, int32 Segments)
{
	Segments = FMath::Max(Segments, 6);
	const float Step = 2.f * PI / Segments;

	float PreviousX = X + Radius;
	float PreviousY = Y;

	for (int32 Index = 1; Index <= Segments; ++Index)
	{
		const float Angle = Index * Step;
		const float NextX = X + Radius * FMath::Cos(Angle);
		const float NextY = Y + Radius * FMath::Sin(Angle);
		DrawLine(PreviousX, PreviousY, NextX, NextY, Colour, Thickness);
		PreviousX = NextX;
		PreviousY = NextY;
	}
}

void AEOPlayerHUD::DrawDiamond(float X, float Y, float Radius, const FLinearColor& Colour, float Thickness)
{
	DrawLine(X, Y - Radius, X + Radius, Y, Colour, Thickness);
	DrawLine(X + Radius, Y, X, Y + Radius, Colour, Thickness);
	DrawLine(X, Y + Radius, X - Radius, Y, Colour, Thickness);
	DrawLine(X - Radius, Y, X, Y - Radius, Colour, Thickness);
}

void AEOPlayerHUD::DrawThreatArc(float CentreX, float CentreY, float Radius, float MidAngleDeg,
	float SpanDeg, const FLinearColor& Colour, float Thickness, bool bDashed)
{
	// Screen space: a bearing of zero is straight up, and bearings increase
	// clockwise, which is how the player reads "to my right".
	const int32 Steps = FMath::Max(FMath::RoundToInt(SpanDeg / 3.f), 3);
	const float StartDeg = MidAngleDeg - SpanDeg * 0.5f;
	const float StepDeg = SpanDeg / Steps;

	auto PointAt = [&](float Degrees, float& OutX, float& OutY)
	{
		const float Radians = FMath::DegreesToRadians(Degrees - 90.f);
		OutX = CentreX + Radius * FMath::Cos(Radians);
		OutY = CentreY + Radius * FMath::Sin(Radians);
	};

	for (int32 Index = 0; Index < Steps; ++Index)
	{
		// Every other segment skipped is the dash: shape carries the same
		// information as the colour, for players who cannot use the colour.
		if (bDashed && (Index % 2) == 1)
		{
			continue;
		}

		float X0 = 0.f;
		float Y0 = 0.f;
		float X1 = 0.f;
		float Y1 = 0.f;
		PointAt(StartDeg + Index * StepDeg, X0, Y0);
		PointAt(StartDeg + (Index + 1) * StepDeg, X1, Y1);

		DrawLine(X0, Y0, X1, Y1, Colour, Thickness);
	}
}

void AEOPlayerHUD::DrawChevrons(float X, float Y, float Size, int32 Lit, int32 Total,
	const FLinearColor& Colour)
{
	const float Thickness = FMath::Max(1.f, Size * 0.14f);
	const float Spacing = Size * 0.75f;

	for (int32 Index = 0; Index < Total; ++Index)
	{
		const float Left = X + Index * Spacing;
		const FLinearColor Use = Index < Lit ? Colour : Ink.CopyWithNewOpacity(0.18f);

		DrawLine(Left + Size * 0.4f, Y, Left, Y + Size * 0.5f, Use, Thickness);
		DrawLine(Left, Y + Size * 0.5f, Left + Size * 0.4f, Y + Size, Use, Thickness);
	}
}

float AEOPlayerHUD::MeasureAltitude(const AEOAircraftPawn& Craft) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.f;
	}

	const FVector Start = Craft.GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 100000.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EOHUDAltitude), false, &Craft);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return -1.f;
	}

	return Hit.Distance * EOHUD::CmToM;
}
