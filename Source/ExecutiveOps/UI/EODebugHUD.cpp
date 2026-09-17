#include "UI/EODebugHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Core/EOPlayerController.h"
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

void AEODebugHUD::DrawLine(const FString& Text, float& Y, const FLinearColor& Color)
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

	DrawLine(TEXT("EXECUTIVE OPS — M0 SKELETON"), Y, FLinearColor(0.2f, 0.9f, 1.f));

	const FString ModeName =
		StaticEnum<EEOControlMode>()->GetNameStringByValue(static_cast<int64>(EOController->GetControlMode()));
	DrawLine(FString::Printf(TEXT("Control:  %s"), *ModeName), Y);

	if (const UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>())
	{
		DrawLine(FString::Printf(TEXT("Mission:  %s"), *Mission->GetMissionStateName()), Y);
	}

	APawn* Pawn = EOController->GetPawn();
	if (Pawn && Pawn->Implements<UEOAircraftControlInterface>())
	{
		const float Speed = IEOAircraftControlInterface::Execute_GetCurrentSpeed(Pawn);
		const bool bHover = IEOAircraftControlInterface::Execute_IsHovering(Pawn);
		const bool bReady = IEOAircraftControlInterface::Execute_IsReadyForDeployment(Pawn);

		// cm/s -> km/h, which is the number that will actually mean something in M1.
		DrawLine(FString::Printf(TEXT("Speed:    %.0f km/h"), Speed * 0.036f), Y);
		DrawLine(FString::Printf(TEXT("Hover:    %s"), bHover ? TEXT("ON") : TEXT("off")), Y);
		DrawLine(FString::Printf(TEXT("Deploy:   %s"), bReady ? TEXT("READY [F]") : TEXT("hold Shift to hover")), Y,
			bReady ? FLinearColor::Green : FLinearColor(0.6f, 0.6f, 0.6f));
	}
	else if (Pawn)
	{
		DrawLine(FString::Printf(TEXT("Speed:    %.0f cm/s"), Pawn->GetVelocity().Size()), Y);
	}

	Y += DebugLineHeight;
	DrawLine(TEXT("EOReset | EOPossessAircraft | EOPossessOperative | EODeploy | EOExtract"), Y,
		FLinearColor(0.6f, 0.6f, 0.6f));
}
