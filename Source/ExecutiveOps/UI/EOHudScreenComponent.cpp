#include "UI/EOHudScreenComponent.h"

#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "ExecutiveOps.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/EODebugHUD.h"

UEOHudScreenComponent::UEOHudScreenComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	// A display, not geometry: it should never block a trace, catch a shot or
	// cast a shadow across the cockpit.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	bUseComplexAsSimpleCollision = true;
}

void UEOHudScreenComponent::OnRegister()
{
	Super::OnRegister();
	BuildScreen();
}

void UEOHudScreenComponent::BuildScreen()
{
	if (bBuilt)
	{
		return;
	}

	// ---- Geometry ----------------------------------------------------------
	// An arc swept about the component's Z, facing back toward the origin, so
	// the component sits at the viewer's eye and the panel curves around them.
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> Colours;

	const int32 Columns = FMath::Max(Segments, 2);
	const float HalfArc = FMath::DegreesToRadians(ArcDegrees) * 0.5f;
	// Height follows the arc, so a 16:9 readout lands on a 16:9 panel.
	const float ArcLength = Radius * FMath::DegreesToRadians(ArcDegrees);
	const float Aspect = (ResolutionX > 0) ? static_cast<float>(ResolutionY) / ResolutionX : 0.5625f;
	const float HalfHeight = ArcLength * Aspect * HeightScale * 0.5f;

	for (int32 i = 0; i <= Columns; ++i)
	{
		const float T = static_cast<float>(i) / Columns;
		const float Angle = FMath::Lerp(-HalfArc, HalfArc, T);

		const float SinA = FMath::Sin(Angle);
		const float CosA = FMath::Cos(Angle);

		// +X is forward, +Y is right: the panel hangs in front of the origin.
		const FVector Outward(CosA, SinA, 0.f);
		const FVector Base = Outward * Radius;

		Vertices.Add(Base + FVector(0.f, 0.f, HalfHeight));
		Vertices.Add(Base - FVector(0.f, 0.f, HalfHeight));

		// Facing the viewer, which is back down the radius.
		Normals.Add(-Outward);
		Normals.Add(-Outward);

		UVs.Add(FVector2D(T, 0.f));
		UVs.Add(FVector2D(T, 1.f));

		const FVector Tangent = FVector(-SinA, CosA, 0.f);
		Tangents.Add(FProcMeshTangent(Tangent, false));
		Tangents.Add(FProcMeshTangent(Tangent, false));

		Colours.Add(FLinearColor::White);
		Colours.Add(FLinearColor::White);
	}

	for (int32 i = 0; i < Columns; ++i)
	{
		const int32 TopLeft = i * 2;
		const int32 BottomLeft = TopLeft + 1;
		const int32 TopRight = TopLeft + 2;
		const int32 BottomRight = TopLeft + 3;

		// Wound so the visible face is the one pointing at the viewer.
		Triangles.Add(TopLeft);
		Triangles.Add(BottomLeft);
		Triangles.Add(TopRight);

		Triangles.Add(TopRight);
		Triangles.Add(BottomLeft);
		Triangles.Add(BottomRight);
	}

	CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colours, Tangents,
		/*bCreateCollision=*/false);

	// ---- Surface -----------------------------------------------------------
	if (!RenderTarget)
	{
		RenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
			this, UCanvasRenderTarget2D::StaticClass(), ResolutionX, ResolutionY);

		if (RenderTarget)
		{
			// Transparent, so only what the HUD actually draws shows on the glass.
			RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
			RenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(
				this, &UEOHudScreenComponent::DrawToRenderTarget);
		}
	}

	if (ScreenMaterial && !ScreenMaterialInstance)
	{
		ScreenMaterialInstance = UMaterialInstanceDynamic::Create(ScreenMaterial, this);
		if (ScreenMaterialInstance)
		{
			ScreenMaterialInstance->SetTextureParameterValue(TEXT("HudTexture"), RenderTarget);
			ScreenMaterialInstance->SetScalarParameterValue(TEXT("Brightness"), Brightness);
			SetMaterial(0, ScreenMaterialInstance);
		}
	}

	bBuilt = true;
}

void UEOHudScreenComponent::RedrawFrom(AEODebugHUD* SourceHud)
{
	if (!SourceHud || !RenderTarget || !IsVisible())
	{
		return;
	}

	// The callback needs the HUD, and the delegate signature cannot carry it.
	PendingHud = SourceHud;
	RenderTarget->UpdateResource();
	PendingHud = nullptr;
}

void UEOHudScreenComponent::DrawToRenderTarget(UCanvas* TargetCanvas, int32 Width, int32 Height_)
{
	if (!PendingHud || !TargetCanvas)
	{
		return;
	}

	// The swap itself lives on the HUD: AHUD::Canvas is protected, so only a
	// subclass can redirect it.
	PendingHud->DrawInto(TargetCanvas);
}
