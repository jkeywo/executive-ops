#include "UI/EOHudWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

FLinearColor UEOHudWidget::ColourForTone(EEOHudTone Tone)
{
	// The same palette the Canvas HUD used, so the two read identically while
	// both exist. Named tones rather than colours in the view model, so a
	// restyle changes this table and nothing upstream.
	switch (Tone)
	{
	case EEOHudTone::Dim:		return FLinearColor(0.55f, 0.58f, 0.62f, 1.f);
	case EEOHudTone::Accent:	return FLinearColor(0.20f, 0.85f, 0.95f, 1.f);
	case EEOHudTone::Caution:	return FLinearColor(0.98f, 0.72f, 0.18f, 1.f);
	case EEOHudTone::Alarm:		return FLinearColor(0.95f, 0.22f, 0.20f, 1.f);
	case EEOHudTone::Ink:
	default:					return FLinearColor(0.92f, 0.94f, 0.96f, 1.f);
	}
}

void UEOHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// HitTestInvisible covers the whole subtree; SelfHitTestInvisible, the
	// default for a UserWidget, only covers the root and leaves every Border and
	// Image the designer added at their own default of Visible.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEOHudWidget::SetText(UTextBlock* Block, const FText& Text, EEOHudTone Tone)
{
	if (!Block)
	{
		return;
	}
	Block->SetText(Text);
	Block->SetColorAndOpacity(FSlateColor(ColourForTone(Tone)));
}

void UEOHudWidget::SetFill(UProgressBar* Bar, float Fraction, EEOHudTone Tone)
{
	if (!Bar)
	{
		return;
	}
	Bar->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
	Bar->SetFillColorAndOpacity(ColourForTone(Tone));
}

void UEOHudWidget::SetShown(UWidget* Widget, bool bShown)
{
	if (!Widget)
	{
		return;
	}
	// Collapsed rather than Hidden, so an absent group gives its space back.
	Widget->SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UEOHudWidget::Apply(const FEOHudViewModel& VM)
{
	Current = VM;

	// 1 Asset
	SetText(AssetCaption, VM.AssetCaption, EEOHudTone::Dim);
	SetText(AssetMode, VM.AssetMode, VM.AssetTone);
	SetText(AssetHeadline, VM.AssetHeadline, VM.AssetTone);
	SetFill(AssetFill, VM.AssetFill, VM.AssetTone);

	// 2 Bearing
	SetText(HeadingText, VM.HeadingText, EEOHudTone::Ink);
	SetText(DirectiveLabel, VM.DirectiveLabel, EEOHudTone::Dim);
	SetText(Directive, VM.Directive, EEOHudTone::Ink);

	// 3 Standing
	SetText(SiteState, VM.SiteState, VM.SiteStateTone);
	SetText(CertaintyText, VM.CertaintyText, VM.SiteStateTone);
	SetFill(CertaintyFill, VM.Certainty, VM.SiteStateTone);

	// 4 Feed - the rows themselves are the Blueprint's, from GetViewModel().
	SetText(FeedCaption, VM.FeedCaption, EEOHudTone::Dim);

	// 5 Focus
	const bool bPrompt = !VM.Prompt.IsEmpty();
	SetShown(PromptGroup, bPrompt);
	SetText(PromptKey, VM.PromptKey, EEOHudTone::Accent);
	SetText(Prompt, VM.Prompt, EEOHudTone::Accent);

	// 6 Self
	SetShown(FlightReadouts, VM.bFlying);
	SetShown(GroundReadouts, !VM.bFlying);
	SetText(SpeedText, VM.SpeedText, EEOHudTone::Ink);
	SetText(AltitudeText, VM.AltitudeText, EEOHudTone::Ink);
	SetText(VerticalSpeedText, VM.VerticalSpeedText, EEOHudTone::Ink);
	SetText(IntegrityText, VM.IntegrityText, VM.IntegrityTone);
	SetFill(IntegrityFill, VM.Integrity, VM.IntegrityTone);

	// 7 Charge
	SetShown(ChargeGroup, VM.bShowCharge);
	SetFill(HoverFill, VM.HoverBlend, VM.HoverBlend > 0.f ? EEOHudTone::Accent : EEOHudTone::Dim);
	SetText(ChargeHint, VM.ChargeHint, EEOHudTone::Dim);

	// 8 Commit
	SetText(CommitCaption, VM.CommitCaption, EEOHudTone::Dim);
	SetText(CommitKey, VM.CommitKey, VM.CommitTone);
	SetText(CommitHeadline, VM.CommitHeadline, VM.CommitTone);
	SetText(CommitDetail, VM.CommitDetail, EEOHudTone::Dim);

	// Anything a text or a bar cannot do is the Blueprint's, with the same values.
	OnApplied(VM);
}
