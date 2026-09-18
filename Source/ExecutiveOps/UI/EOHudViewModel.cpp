#include "UI/EOHudViewModel.h"

#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Character/EOTraversalComponent.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "Core/EOPlayerController.h"
#include "Feedback/EOFeedbackSubsystem.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Interfaces/EOExtractionInterface.h"
#include "Mission/EOInteractableInterface.h"
#include "Mission/EOExtractionZone.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOMissionSubsystem.h"
#include "UI/EOHUDUnits.h"
#include "UI/EOHudState.h"

#include "Engine/World.h"

namespace
{
	FText Upper(const FString& S) { return FText::FromString(S.ToUpper()); }
	FText Str(const TCHAR* S) { return FText::FromString(S); }

	// A macro rather than a function, because Printf checks its format string at
	// compile time and will not accept one passed in at runtime.
	#define Fmt(Format, Value) FText::FromString(FString::Printf(Format, Value))

	/** Metres from the craft to whatever is under it, or -1 with nothing below. */
	float MeasureAltitude(const AEOAircraftPawn& Craft)
	{
		const UWorld* World = Craft.GetWorld();
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
}

// ---- Reductions on plain inputs ----------------------------------------------

FText FEOHudViewModel::DirectiveFor(bool bHasMission, EEOMissionState MissionState, const FString& SiteName)
{
	if (!bHasMission)
	{
		return Str(TEXT("NO MISSION CONTROL"));
	}

	switch (MissionState)
	{
	case EEOMissionState::Inactive:			return Str(TEXT("NO MISSION SELECTED"));
	case EEOMissionState::InFlight:			return FText::FromString(FString::Printf(TEXT("APPROACH %s"), *SiteName.ToUpper()));
	case EEOMissionState::Deploying:		return Str(TEXT("DEPLOYING"));
	case EEOMissionState::OnGround:			return Str(TEXT("SECURE THE OBJECTIVE"));
	case EEOMissionState::ObjectiveComplete:	return Str(TEXT("OBJECTIVE SECURE - EXTRACT"));
	case EEOMissionState::Extracting:		return Str(TEXT("EXTRACTION INBOUND"));
	case EEOMissionState::Complete:			return Str(TEXT("MISSION COMPLETE"));
	case EEOMissionState::Failed:			return Str(TEXT("OPERATIVE DOWN"));
	default:					return Str(TEXT("--"));
	}
}

int32 FEOHudViewModel::StandingTierFor(EEOGuardState GuardState, float Detection)
{
	switch (GuardState)
	{
	case EEOGuardState::Alerted:	return 3;
	case EEOGuardState::Searching:	return 2;
	case EEOGuardState::Suspicious:	return 1;
	case EEOGuardState::Patrolling:	return Detection > 0.f ? 1 : 0;
	default:			return 0;
	}
}

FText FEOHudViewModel::StandingTextFor(int32 Tier, bool bAnyGuards)
{
	if (!bAnyGuards)
	{
		return Str(TEXT("NO CONTACT"));
	}
	switch (Tier)
	{
	case 3:	return Str(TEXT("ALERTED"));
	case 2:	return Str(TEXT("SEARCHING"));
	case 1:	return Str(TEXT("SUSPICIOUS"));
	default:	return Str(TEXT("UNAWARE"));
	}
}

EEOHudTone FEOHudViewModel::StandingToneFor(int32 Tier, bool bAnyGuards)
{
	if (!bAnyGuards)
	{
		return EEOHudTone::Dim;
	}
	switch (Tier)
	{
	case 3:	return EEOHudTone::Alarm;
	case 2:	return EEOHudTone::Caution;
	case 1:	return EEOHudTone::Ink;
	default:	return EEOHudTone::Dim;
	}
}

EEOHudTone FEOHudViewModel::ToneForGuard(EEOGuardState GuardState)
{
	switch (GuardState)
	{
	case EEOGuardState::Alerted:	return EEOHudTone::Alarm;
	case EEOGuardState::Searching:
	case EEOGuardState::Suspicious:	return EEOHudTone::Caution;
	default:			return EEOHudTone::Ink;
	}
}

FText FEOHudViewModel::ContactTextFor(EEOGuardState GuardState)
{
	switch (GuardState)
	{
	case EEOGuardState::Alerted:	return Str(TEXT("ALERTED"));
	case EEOGuardState::Searching:	return Str(TEXT("SEARCHING"));
	case EEOGuardState::Suspicious:	return Str(TEXT("SUSPICIOUS"));
	default:			return Str(TEXT("LOOKING"));
	}
}

FText FEOHudViewModel::StanceFor(bool bDead, bool bTakedown, bool bDeploying, const FString& TraversalName,
	bool bSliding, bool bSprinting, bool bCrouched, bool bAiming)
{
	// Ordered by how much each one overrides the rest: being down trumps
	// everything, a verb in progress trumps a posture.
	if (bDead)			{ return Str(TEXT("DOWN")); }
	if (bTakedown)			{ return Str(TEXT("TAKEDOWN")); }
	if (bDeploying)			{ return Str(TEXT("DROPPING")); }
	if (!TraversalName.IsEmpty())	{ return Upper(TraversalName); }
	if (bSliding)			{ return Str(TEXT("SLIDING")); }
	if (bSprinting)			{ return Str(TEXT("SPRINTING")); }
	if (bCrouched)			{ return Str(TEXT("CROUCHED")); }
	if (bAiming)			{ return Str(TEXT("AIMING")); }
	return Str(TEXT("STANDING"));
}

EEOHudExtractionStatus FEOHudViewModel::ExtractionStatusFor(bool bInbound, bool bHasRoute, bool bAvailable, bool bInZone)
{
	if (bInbound)		{ return EEOHudExtractionStatus::Inbound; }
	if (!bHasRoute)		{ return EEOHudExtractionStatus::NoRoute; }
	if (!bAvailable)	{ return EEOHudExtractionStatus::Sealed; }
	if (bInZone)		{ return EEOHudExtractionStatus::Extract; }
	return EEOHudExtractionStatus::PadOpen;
}

// ---- The frame -----------------------------------------------------------------

FEOHudViewModel FEOHudViewModel::Build(const FEOHUDState& S)
{
	FEOHudViewModel VM;
	if (!S.Controller || !S.Pawn)
	{
		return VM;
	}

	VM.bFlying = S.bFlying;
	const FVector Here = S.Pawn->GetActorLocation();
	const float Ref = EOHUD::ReferenceYaw(*S.Pawn, *S.Controller, S.bFlying);

	// ---- 1 Asset
	if (S.bFlying && S.Aircraft)
	{
		const bool bHover = IEOAircraftControlInterface::Execute_IsHovering(S.Aircraft);
		VM.AssetCaption = Str(TEXT("AIRFRAME"));
		VM.AssetMode = Str(bHover ? TEXT("HOVER") : TEXT("FLIGHT"));
		VM.AssetHeadline = Str(bHover ? TEXT("STATION KEEPING") : TEXT("FORWARD FLIGHT"));
		VM.AssetFill = S.Aircraft->GetThrustAlpha();
		VM.AssetTone = bHover ? EEOHudTone::Accent : EEOHudTone::Ink;
	}
	else
	{
		const bool bInbound = S.Controller->IsExtractionInbound();
		VM.AssetCaption = Str(TEXT("OVERWATCH"));
		VM.AssetMode = Str(bInbound ? TEXT("INBOUND") : TEXT("STANDBY"));
		VM.AssetTone = bInbound ? EEOHudTone::Accent : EEOHudTone::Dim;
		if (bInbound)
		{
			VM.AssetHeadline = Fmt(TEXT("%.0f M"), S.Controller->GetExtractionDistance());
		}
		else if (S.Aircraft)
		{
			VM.AssetHeadline = Fmt(TEXT("%.0f M"), FVector::Dist(S.Aircraft->GetActorLocation(), Here) * EOHUD::CmToM);
		}
		else
		{
			VM.AssetHeadline = Str(TEXT("--"));
		}
	}

	// ---- 2 Bearing
	VM.Heading = FMath::RoundToInt(FRotator::ClampAxis(Ref)) % 360;
	VM.HeadingText = FText::FromString(FString::Printf(TEXT("%03d"), VM.Heading));
	if (S.Site)
	{
		VM.bHasSiteBearing = true;
		VM.SiteRelativeBearing = EOHUD::RelativeBearing(Here, S.Site->GetHoverPoint(), Ref);
	}
	if (S.Extraction && IEOExtractionInterface::Execute_IsExtractionAvailable(S.Extraction))
	{
		VM.bHasExtractionBearing = true;
		VM.ExtractionRelativeBearing = EOHUD::RelativeBearing(Here, S.Extraction->GetActorLocation(), Ref);
	}
	VM.DirectiveLabel = Str(S.bFlying ? TEXT("DIRECTIVE") : TEXT("OBJECTIVE"));
	VM.Directive = DirectiveFor(S.Mission != nullptr,
		S.Mission ? S.Mission->GetMissionState() : EEOMissionState::Inactive,
		S.Site ? S.Site->GetDisplayName().ToString() : TEXT("UNKNOWN SITE"));

	// ---- 3 Standing
	int32 Tier = 0;
	float MaxDetection = 0.f;
	for (const AEOGuardCharacter* Guard : S.Guards)
	{
		MaxDetection = FMath::Max(MaxDetection, Guard->GetDetectionAlpha());
		Tier = FMath::Max(Tier, StandingTierFor(Guard->GetGuardState(), Guard->GetDetectionAlpha()));
	}
	const bool bAnyGuards = S.Guards.Num() > 0;
	VM.SiteState = StandingTextFor(Tier, bAnyGuards);
	VM.SiteStateTone = StandingToneFor(Tier, bAnyGuards);
	VM.Certainty = MaxDetection;
	VM.CertaintyText = Fmt(TEXT("%.0f%%"), MaxDetection * 100.f);

	// ---- 4 Feed
	if (S.bFlying)
	{
		if (S.Site)
		{
			FEOHudFeedRow Row;
			Row.Label = Upper(S.Site->GetDisplayName().ToString());
			Row.Tag = Str(TEXT("CONTRACT"));
			Row.DistanceM = FVector::Dist(Here, S.Site->GetHoverPoint()) * EOHUD::CmToM;
			Row.Tone = EEOHudTone::Accent;
			VM.Feed.Add(Row);
		}
		if (VM.bHasExtractionBearing)
		{
			FEOHudFeedRow Row;
			Row.Label = Str(TEXT("EXTRACTION PAD"));
			Row.Tag = Str(TEXT("PICKUP"));
			Row.DistanceM = FVector::Dist(Here, S.Extraction->GetActorLocation()) * EOHUD::CmToM;
			Row.Tone = EEOHudTone::Ink;
			VM.Feed.Add(Row);
		}
		VM.FeedCaption = FText::FromString(FString::Printf(TEXT("SCAN - %d RETURN%s"),
			VM.Feed.Num(), VM.Feed.Num() == 1 ? TEXT("") : TEXT("S")));
	}
	else
	{
		for (const AEOGuardCharacter* Guard : S.Guards)
		{
			if (Guard->GetGuardState() == EEOGuardState::Patrolling && Guard->GetDetectionAlpha() <= 0.f)
			{
				continue;
			}
			FEOHudFeedRow Row;
			Row.Label = ContactTextFor(Guard->GetGuardState());
			Row.Tag = Str(TEXT("CONTACT"));
			Row.DistanceM = FVector::Dist(Here, Guard->GetActorLocation()) * EOHUD::CmToM;
			Row.Fill = Guard->GetDetectionAlpha();
			Row.Tone = ToneForGuard(Guard->GetGuardState());
			VM.Feed.Add(Row);
		}
		VM.FeedCaption = Str(TEXT("CONTACT LOG"));
	}

	// ---- 5 Focus
	if (S.bFlying && S.Aircraft)
	{
		const FVector Velocity = S.Aircraft->GetVelocity();
		if (!Velocity.IsNearlyZero())
		{
			VM.bHasFlightReticle = true;
			VM.FlightReticleWorld = S.Aircraft->GetActorLocation() + Velocity.GetSafeNormal() * 20000.f;
		}
	}
	else if (S.Operative)
	{
		VM.bAiming = S.Operative->IsAiming();
		for (const AEOGuardCharacter* Guard : S.Guards)
		{
			FEOHudThreat Threat;
			Threat.RelativeBearing = EOHUD::RelativeBearing(Here, Guard->GetActorLocation(), Ref);
			Threat.Detection = Guard->GetDetectionAlpha();
			Threat.Tone = ToneForGuard(Guard->GetGuardState());
			VM.Threats.Add(Threat);
		}
		if (S.Operative->FindTakedownTarget())
		{
			VM.PromptKey = Str(TEXT("F"));
			VM.Prompt = Str(TEXT("TAKEDOWN"));
		}
		else if (AActor* Interactable = S.Operative->FindInteractable())
		{
			VM.Prompt = IEOInteractableInterface::Execute_GetInteractionPrompt(Interactable);
		}
	}

	// ---- 6 Self
	if (S.bFlying && S.Aircraft)
	{
		const float Altitude = MeasureAltitude(*S.Aircraft);
		VM.SpeedText = Fmt(TEXT("%.0f"), IEOAircraftControlInterface::Execute_GetCurrentSpeed(S.Aircraft) * EOHUD::CmsToKph);
		VM.AltitudeText = Altitude < 0.f ? Str(TEXT("--")) : Fmt(TEXT("%.0f"), Altitude);
		VM.VerticalSpeedText = Fmt(TEXT("%+.0f"), S.Aircraft->GetVelocity().Z * EOHUD::CmToM);
	}
	else if (S.Operative)
	{
		const UEOHealthComponent* Health = S.Operative->GetHealth();
		VM.Integrity = Health ? Health->GetHealthFraction() : 1.f;

		FString TraversalName;
		if (const UEOTraversalComponent* Traversal = S.Operative->GetTraversal())
		{
			if (Traversal->IsTraversing())
			{
				TraversalName = StaticEnum<EEOTraversalType>()
					->GetNameStringByValue(static_cast<int64>(Traversal->GetActiveType()));
			}
		}
		const FText Stance = StanceFor(S.Operative->IsDead(), S.Operative->IsPerformingTakedown(),
			S.Operative->IsDeploying(), TraversalName, S.Operative->IsSliding(),
			S.Operative->IsSprinting(), S.Operative->bIsCrouched, S.Operative->IsAiming());

		VM.IntegrityText = FText::FromString(FString::Printf(TEXT("%.0f%% - %s"),
			VM.Integrity * 100.f, *Stance.ToString()));
		VM.IntegrityTone = VM.Integrity < 0.4f ? EEOHudTone::Alarm : EEOHudTone::Ink;
	}

	// ---- 7 Charge
	if (S.bFlying && S.Aircraft)
	{
		VM.bShowCharge = true;
		VM.HoverBlend = S.Aircraft->GetHoverBlend();
		VM.ChargeHint = Str(TEXT("SPACE TOGGLES"));
	}

	// ---- 8 Commit
	if (S.bFlying)
	{
		const FString Blocker = S.Controller->GetDeploymentBlocker();
		VM.CommitCaption = Str(TEXT("INSERTION"));
		VM.CommitKey = Str(TEXT("F"));
		VM.CommitHeadline = Str(TEXT("DEPLOY"));
		VM.CommitDetail = FText::FromString(Blocker);
		VM.CommitTone = Blocker.IsEmpty() ? EEOHudTone::Accent : EEOHudTone::Dim;
	}
	else
	{
		VM.CommitCaption = Str(TEXT("EXTRACTION"));
		const bool bAvailable = S.Extraction && IEOExtractionInterface::Execute_IsExtractionAvailable(S.Extraction);
		const bool bInZone = S.Extraction && S.Extraction->IsWithinZone(S.Pawn);
		VM.Extraction = ExtractionStatusFor(S.Controller->IsExtractionInbound(),
			S.Extraction != nullptr, bAvailable, bInZone);

		switch (VM.Extraction)
		{
		case EEOHudExtractionStatus::Inbound:
			VM.CommitHeadline = Str(TEXT("INBOUND"));
			VM.CommitDetail = Fmt(TEXT("%.0f M TO PICKUP"), S.Controller->GetExtractionDistance());
			VM.CommitTone = EEOHudTone::Accent;
			break;
		case EEOHudExtractionStatus::NoRoute:
			VM.CommitHeadline = Str(TEXT("NO ROUTE"));
			VM.CommitTone = EEOHudTone::Dim;
			break;
		case EEOHudExtractionStatus::Sealed:
			VM.CommitHeadline = Str(TEXT("SEALED"));
			VM.CommitDetail = Str(TEXT("OBJECTIVE INCOMPLETE"));
			VM.CommitTone = EEOHudTone::Dim;
			break;
		case EEOHudExtractionStatus::Extract:
			VM.CommitKey = Str(TEXT("E"));
			VM.CommitHeadline = Str(TEXT("EXTRACT"));
			VM.CommitTone = EEOHudTone::Accent;
			break;
		case EEOHudExtractionStatus::PadOpen:
			VM.CommitHeadline = Str(TEXT("PAD OPEN"));
			VM.CommitDetail = Fmt(TEXT("%.0f M"),
				FVector::Dist(S.Extraction->GetActorLocation(), Here) * EOHUD::CmToM);
			VM.CommitTone = EEOHudTone::Accent;
			break;
		}
	}

	// ---- Screen feedback
	if (const UEOFeedbackSubsystem* Feedback = UEOFeedbackSubsystem::Get(S.Controller))
	{
		VM.Vignette = Feedback->GetVignetteAlpha();
		FVector Direction;
		float Alpha = 0.f;
		if (Feedback->GetDamageDirection(Direction, Alpha) && Alpha > 0.f)
		{
			VM.DamageAlpha = Alpha;
			VM.DamageRelativeBearing = EOHUD::RelativeBearing(FVector::ZeroVector, Direction,
				S.Controller->GetControlRotation().Yaw);
		}
	}

	return VM;
}

#undef Fmt
