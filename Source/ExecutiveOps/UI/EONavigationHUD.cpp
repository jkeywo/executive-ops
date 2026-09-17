#include "UI/EONavigationHUD.h"
#include "UI/EOHUDUnits.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Core/EOPlayerController.h"
#include "Mission/EOMissionSite.h"
#include "Mission/EOMissionSubsystem.h"

namespace
{
	// The frame's accent: the waypoint and the compass marker point at the same
	// site, so they are not allowed to disagree about what colour it is.
	const FLinearColor ObjectiveColour(0.275f, 0.847f, 0.745f);
	const FLinearColor MapInkColour(0.35f, 0.4f, 0.45f);
	const FLinearColor PlayerColour(0.2f, 0.9f, 1.f);

	/** cm -> metres, for a distance readout a pilot can actually use. */
}

AEONavigationHUD::AEONavigationHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEONavigationHUD::DrawHUD()
{
	// The debug readout draws first; navigation sits on top of it.
	Super::DrawHUD();

	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	APawn* Viewer = PlayerOwner->GetPawn();
	if (!Viewer)
	{
		return;
	}

	const UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	AEOMissionSite* Site = Mission ? Mission->GetSelectedSite() : nullptr;

	if (Site)
	{
		DrawWaypoint(*Site, Viewer->GetActorLocation());
	}

	if (bMapVisible)
	{
		DrawTacticalMap(Site, *Viewer);
	}
}

void AEONavigationHUD::DrawWaypoint(const AEOMissionSite& Site, const FVector& ViewerLocation)
{
	const FVector Target = Site.GetHoverPoint();
	const float DistanceM = FVector::Dist(ViewerLocation, Target) * EOHUD::CmToM;

	const FVector Screen = Project(Target);

	// Project() returns a position behind the camera with a negative Z, which would
	// otherwise draw a waypoint for something the player is flying away from.
	const bool bOnScreen = Screen.Z > 0.f
		&& Screen.X >= 0.f && Screen.X <= Canvas->SizeX
		&& Screen.Y >= 0.f && Screen.Y <= Canvas->SizeY;

	if (bOnScreen)
	{
		const float X = Screen.X;
		const float Y = Screen.Y;
		const float R = 14.f;

		// Diamond outline.
		DrawLine(X, Y - R, X + R, Y, ObjectiveColour, 2.f);
		DrawLine(X + R, Y, X, Y + R, ObjectiveColour, 2.f);
		DrawLine(X, Y + R, X - R, Y, ObjectiveColour, 2.f);
		DrawLine(X - R, Y, X, Y - R, ObjectiveColour, 2.f);

		DrawText(FString::Printf(TEXT("%s  %.0fm"), *Site.GetDisplayName().ToString(), DistanceM),
			ObjectiveColour, X + R + 6.f, Y - 8.f, GEngine->GetSmallFont(), 1.1f);
		return;
	}

	// Off-screen: clamp an arrow to the screen edge pointing the way to turn.
	const FVector2D ScreenCentre(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	FVector2D Direction(Screen.X - ScreenCentre.X, Screen.Y - ScreenCentre.Y);

	// Behind the camera the projection mirrors, so flip it back.
	if (Screen.Z <= 0.f)
	{
		Direction = -Direction;
	}

	if (Direction.IsNearlyZero())
	{
		Direction = FVector2D(0.f, -1.f);
	}
	Direction.Normalize();

	// Intersect the bearing ray with the screen rect. Scaling the direction by the
	// half-extents instead would trace an ellipse, putting the arrow short of the
	// edge and at a different bearing from the one it points along.
	const float EdgeInset = 70.f;
	const float HalfW = ScreenCentre.X - EdgeInset;
	const float HalfH = ScreenCentre.Y - EdgeInset;

	const float TimeToVerticalEdge = FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER
		? HalfW / FMath::Abs(Direction.X) : TNumericLimits<float>::Max();
	const float TimeToHorizontalEdge = FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER
		? HalfH / FMath::Abs(Direction.Y) : TNumericLimits<float>::Max();

	const FVector2D Edge = ScreenCentre + Direction * FMath::Min(TimeToVerticalEdge, TimeToHorizontalEdge);

	// Triangle pointing along Direction.
	const FVector2D Perp(-Direction.Y, Direction.X);
	const FVector2D Tip = Edge + Direction * 18.f;
	const FVector2D Left = Edge - Direction * 8.f + Perp * 11.f;
	const FVector2D Right = Edge - Direction * 8.f - Perp * 11.f;

	DrawLine(Tip.X, Tip.Y, Left.X, Left.Y, ObjectiveColour, 2.f);
	DrawLine(Left.X, Left.Y, Right.X, Right.Y, ObjectiveColour, 2.f);
	DrawLine(Right.X, Right.Y, Tip.X, Tip.Y, ObjectiveColour, 2.f);

	DrawText(FString::Printf(TEXT("%.0fm"), DistanceM),
		ObjectiveColour, Edge.X - 18.f, Edge.Y + 20.f, GEngine->GetSmallFont(), 1.1f);
}

void AEONavigationHUD::CacheMapGeometry()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		// Do not latch the cache on a failed scan: the next frame retries.
		return;
	}

	MapGeometry.Reset();

	// The greybox city is all static mesh actors and never moves, so one pass at
	// startup is enough and keeps the map free per frame.
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		const UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		if (!Mesh || !Mesh->GetStaticMesh())
		{
			continue;
		}

		const FBoxSphereBounds Bounds = Mesh->Bounds;

		// Plot things that stand up. A plan view wants building footprints; the
		// ground plane and flat overhead slabs are either the whole map or noise.
		// Keyed to height rather than footprint size, because the ground's half
		// extent is smaller than the map radius and would otherwise pass.
		if (Bounds.BoxExtent.Z < MinPlottedHeight)
		{
			continue;
		}

		FMapFootprint Footprint;
		Footprint.Centre = FVector2D(Bounds.Origin.X, Bounds.Origin.Y);
		Footprint.HalfExtent = FVector2D(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
		MapGeometry.Add(Footprint);
	}

	LastCacheTime = World->GetTimeSeconds();
	bGeometryCached = MapGeometry.Num() > 0;
}

void AEONavigationHUD::DrawTacticalMap(const AEOMissionSite* Site, const APawn& Viewer)
{
	// Rescanned periodically as well as on first use, because level geometry can
	// stream in after the HUD has already drawn a frame.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (!bGeometryCached || Now - LastCacheTime > GeometryRefreshInterval)
	{
		CacheMapGeometry();
	}

	const float Size = MapScreenSize;
	const float Left = Canvas->SizeX - Size - MapScreenMargin;
	const float Top = MapScreenMargin + MapTopOffset;
	const FVector2D Centre(Left + Size * 0.5f, Top + Size * 0.5f);

	// The map is north-up and centred on the player, so it stays readable while
	// the craft yaws. World +X draws up, world +Y draws right.
	const FVector ViewerLocation = Viewer.GetActorLocation();
	const float Scale = (Size * 0.5f) / FMath::Max(MapWorldExtent, 1.f);

	auto WorldToMap = [&](const FVector2D& World) -> FVector2D
	{
		const FVector2D Offset = World - FVector2D(ViewerLocation.X, ViewerLocation.Y);
		return FVector2D(Centre.X + Offset.Y * Scale, Centre.Y - Offset.X * Scale);
	};

	auto InsideMap = [&](const FVector2D& P) -> bool
	{
		return P.X >= Left && P.X <= Left + Size && P.Y >= Top && P.Y <= Top + Size;
	};

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), Left, Top, Size, Size);

	// Buildings as outlined footprints.
	for (const FMapFootprint& Footprint : MapGeometry)
	{
		const FVector2D Min = WorldToMap(Footprint.Centre - Footprint.HalfExtent);
		const FVector2D Max = WorldToMap(Footprint.Centre + Footprint.HalfExtent);

		const FVector2D TopLeft(FMath::Min(Min.X, Max.X), FMath::Min(Min.Y, Max.Y));
		const FVector2D BottomRight(FMath::Max(Min.X, Max.X), FMath::Max(Min.Y, Max.Y));

		if (BottomRight.X < Left || TopLeft.X > Left + Size ||
			BottomRight.Y < Top || TopLeft.Y > Top + Size)
		{
			continue;
		}

		// Clip to the map box so buildings do not bleed across the screen.
		const float X0 = FMath::Clamp(TopLeft.X, Left, Left + Size);
		const float Y0 = FMath::Clamp(TopLeft.Y, Top, Top + Size);
		const float X1 = FMath::Clamp(BottomRight.X, Left, Left + Size);
		const float Y1 = FMath::Clamp(BottomRight.Y, Top, Top + Size);

		DrawRect(MapInkColour.CopyWithNewOpacity(0.8f), X0, Y0, FMath::Max(X1 - X0, 1.f), FMath::Max(Y1 - Y0, 1.f));
	}

	// Mission site.
	if (Site)
	{
		const FVector Hover = Site->GetHoverPoint();
		const FVector2D P = WorldToMap(FVector2D(Hover.X, Hover.Y));
		if (InsideMap(P))
		{
			const float R = 6.f;
			DrawLine(P.X - R, P.Y - R, P.X + R, P.Y + R, ObjectiveColour, 2.f);
			DrawLine(P.X - R, P.Y + R, P.X + R, P.Y - R, ObjectiveColour, 2.f);
		}
	}

	// The player, as a triangle pointing along the craft's heading.
	const float Yaw = FMath::DegreesToRadians(Viewer.GetActorRotation().Yaw);
	// World forward (cos, sin) maps to screen (sin, -cos) under the transform above.
	const FVector2D Facing(FMath::Sin(Yaw), -FMath::Cos(Yaw));
	const FVector2D Perp(-Facing.Y, Facing.X);

	const FVector2D Tip = Centre + Facing * 9.f;
	const FVector2D L = Centre - Facing * 5.f + Perp * 6.f;
	const FVector2D R2 = Centre - Facing * 5.f - Perp * 6.f;

	DrawLine(Tip.X, Tip.Y, L.X, L.Y, PlayerColour, 2.f);
	DrawLine(L.X, L.Y, R2.X, R2.Y, PlayerColour, 2.f);
	DrawLine(R2.X, R2.Y, Tip.X, Tip.Y, PlayerColour, 2.f);

	DrawText(TEXT("DISTRICT [M]"), MapInkColour, Left + 6.f, Top + Size - 16.f, GEngine->GetSmallFont(), 1.f);
}
