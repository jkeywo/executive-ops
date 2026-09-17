#include "UI/EODebugHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Core/EOPlayerController.h"
#include "Aircraft/EOAircraftPawn.h"
#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"
#include "Combat/EOHealthComponent.h"
#include "EngineUtils.h"
#include "Interfaces/EOAircraftControlInterface.h"
#include "Mission/EOMissionSubsystem.h"

namespace
{
	constexpr float DebugLeftMargin = 24.f;
	constexpr float DebugTopMargin = 24.f;
	constexpr float DebugLineHeight = 18.f;
}

AEODebugHUD::AEODebugHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEODebugHUD::DrawTextLine(const FString& Text, float& Y, const FLinearColor& Color)
{
	DrawText(Text, Color, DebugLeftMargin, Y, GEngine->GetSmallFont(), 1.2f);
	Y += DebugLineHeight;
}

void AEODebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bDebugVisible || !Canvas)
	{
		return;
	}

	const AEOPlayerController* EOController = Cast<AEOPlayerController>(PlayerOwner);
	if (!EOController)
	{
		return;
	}

	float Y = DebugTopMargin;

	DrawTextLine(TEXT("EXECUTIVE OPS"), Y, FLinearColor(0.2f, 0.9f, 1.f));

	const FString ModeName =
		StaticEnum<EEOControlMode>()->GetNameStringByValue(static_cast<int64>(EOController->GetControlMode()));
	DrawTextLine(FString::Printf(TEXT("Control:  %s"), *ModeName), Y);

	if (const UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		DrawTextLine(FString::Printf(TEXT("Mission:  %s"), *Mission->GetMissionStateName()), Y);
	}

	APawn* Pawn = EOController->GetPawn();
	if (Pawn && Pawn->Implements<UEOAircraftControlInterface>())
	{
		const float Speed = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Pawn);
		const bool bHover = IEOAircraftControlInterface::Execute_IsHovering(Pawn);

		// cm/s -> km/h, the number that actually means something while flying.
		DrawTextLine(FString::Printf(TEXT("Speed:    %.0f km/h"), Speed * 0.036f), Y);

		if (const AEOAircraftPawn* Craft = Cast<AEOAircraftPawn>(Pawn))
		{
			// A bar, because the hover transition is a blend and a boolean hides that.
			const int32 Filled = FMath::RoundToInt(Craft->GetHoverBlend() * 10.f);
			const FString Bar = FString::ChrN(Filled, TEXT('=')) + FString::ChrN(10 - Filled, TEXT('.'));
			DrawTextLine(FString::Printf(TEXT("Hover:    [%s] %s"), *Bar, bHover ? TEXT("HELD") : TEXT("")), Y);
			DrawTextLine(FString::Printf(TEXT("Thrust:   %.0f%%"), Craft->GetThrustAlpha() * 100.f), Y);
		}
		else
		{
			DrawTextLine(FString::Printf(TEXT("Hover:    %s"), bHover ? TEXT("ON") : TEXT("off")), Y);
		}
		// One source of truth: the prompt is whatever the deployment gate says.
		const FString Blocker = EOController->GetDeploymentBlocker();
		DrawTextLine(FString::Printf(TEXT("Deploy:   %s"),
			Blocker.IsEmpty() ? TEXT("READY [F]") : *Blocker), Y,
			Blocker.IsEmpty() ? FLinearColor::Green : FLinearColor(0.6f, 0.6f, 0.6f));
	}
	else if (const AEOOperativeCharacter* Operative = Cast<AEOOperativeCharacter>(Pawn))
	{
		DrawTextLine(FString::Printf(TEXT("Speed:    %.0f cm/s"), Pawn->GetVelocity().Size()), Y);

		if (const UEOHealthComponent* OperativeHealth = Operative->GetHealth())
		{
			const float Fraction = OperativeHealth->GetHealthFraction();
			const int32 Filled = FMath::RoundToInt(Fraction * 10.f);
			const FString Bar = FString::ChrN(Filled, TEXT('=')) + FString::ChrN(10 - Filled, TEXT('.'));

			DrawTextLine(FString::Printf(TEXT("Health:   [%s] %.0f"), *Bar, OperativeHealth->GetHealth()), Y,
				OperativeHealth->IsDead() ? FLinearColor::Red
					: (Fraction < 0.4f ? FLinearColor(1.f, 0.5f, 0.1f) : FLinearColor::White));
		}

		if (Operative->IsAiming())
		{
			DrawTextLine(TEXT("Weapon:   AIMING"), Y, FLinearColor(0.2f, 0.9f, 1.f));
		}

		// The takedown prompt is driven by the same test the action uses, so it
		// can never offer a kill the input would then refuse.
		if (Operative->FindTakedownTarget())
		{
			DrawTextLine(TEXT("TAKEDOWN [F]"), Y, FLinearColor::Green);
		}

		// Guard awareness: what the player needs to read to know if they are
		// getting away with it.
		for (TActorIterator<AEOGuardCharacter> It(GetWorld()); It; ++It)
		{
			const AEOGuardCharacter* Guard = *It;
			if (!Guard)
			{
				continue;
			}

			const FString StateName = StaticEnum<EEOGuardState>()
				->GetNameStringByValue(static_cast<int64>(Guard->GetGuardState()));

			const int32 Filled = FMath::RoundToInt(Guard->GetDetectionAlpha() * 10.f);
			const FString Bar = FString::ChrN(Filled, TEXT('!')) + FString::ChrN(10 - Filled, TEXT('.'));

			FLinearColor Colour = FLinearColor(0.6f, 0.6f, 0.6f);
			if (Guard->IsDead())					{ Colour = FLinearColor(0.4f, 0.4f, 0.4f); }
			else if (Guard->GetGuardState() == EEOGuardState::Alerted)	{ Colour = FLinearColor::Red; }
			else if (Guard->GetDetectionAlpha() > 0.f)					{ Colour = FLinearColor::Yellow; }

			DrawTextLine(FString::Printf(TEXT("Guard:    %-11s [%s]"), *StateName, *Bar), Y, Colour);
		}
	}
	else if (Pawn)
	{
		DrawTextLine(FString::Printf(TEXT("Speed:    %.0f cm/s"), Pawn->GetVelocity().Size()), Y);
	}

	Y += DebugLineHeight;
	DrawTextLine(TEXT("EOReset | EOPossessAircraft | EOPossessOperative | EODeploy | EOExtract"), Y,
		FLinearColor(0.6f, 0.6f, 0.6f));
}
