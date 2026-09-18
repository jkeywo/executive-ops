#include "Mission/EOMissionSite.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "ExecutiveOps.h"

AEOMissionSite::AEOMissionSite()
{
	PrimaryActorTick.bCanEverTick = true;

	HoverVolume = CreateDefaultSubobject<USphereComponent>(TEXT("HoverVolume"));
	HoverVolume->InitSphereRadius(1500.f);
	// Query-only: the aircraft must be able to fly straight through it.
	HoverVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HoverVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	RootComponent = HoverVolume;

	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetRelativeLocation(FVector(0.f, 0.f, 600.f));
	Marker->SetRelativeScale3D(FVector(2.f, 2.f, 2.f));

	InsertionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("InsertionPoint"));
	InsertionPoint->SetupAttachment(RootComponent);
	InsertionPoint->SetRelativeLocation(FVector(0.f, 0.f, -1400.f));
}

void AEOMissionSite::BeginPlay()
{
	Super::BeginPlay();

	MarkerBaseZ = Marker ? Marker->GetRelativeLocation().Z : 0.f;
}

void AEOMissionSite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Marker)
	{
		return;
	}

	// A slowly spinning, bobbing marker is the cheapest thing that reads as
	// "here" against a city of identical grey boxes.
	Elapsed += DeltaSeconds;
	Marker->AddLocalRotation(FRotator(0.f, MarkerSpinRate * DeltaSeconds, 0.f));

	FVector Location = Marker->GetRelativeLocation();
	Location.Z = MarkerBaseZ + FMath::Sin(Elapsed * MarkerBobRate) * MarkerBobHeight;
	Marker->SetRelativeLocation(Location);
}

FVector AEOMissionSite::GetHoverPoint() const
{
	return HoverVolume ? HoverVolume->GetComponentLocation() : GetActorLocation();
}

float AEOMissionSite::GetHoverRadius() const
{
	return HoverVolume ? HoverVolume->GetScaledSphereRadius() : 0.f;
}

bool AEOMissionSite::IsWithinHoverVolume(const AActor* Actor) const
{
	if (!Actor || !HoverVolume)
	{
		return false;
	}

	// Distance test rather than overlap events: the aircraft moves by sweeping a
	// box each frame, and overlap bookkeeping across teleports is easy to get wrong.
	const float Radius = GetHoverRadius();
	return FVector::DistSquared(Actor->GetActorLocation(), GetHoverPoint()) <= Radius * Radius;
}

FTransform AEOMissionSite::GetInsertionTransform() const
{
	return InsertionPoint ? InsertionPoint->GetComponentTransform() : GetActorTransform();
}

bool AEOMissionSite::IsArenaReady() const
{
	if (!HasArena())
	{
		return true;
	}

	return ArenaStreaming && ArenaStreaming->IsLevelVisible();
}

bool AEOMissionSite::IsWithinArenaLoadRadius(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	const float Radius = GetHoverRadius() * ArenaLoadRadiusScale;
	return FVector::DistSquared(Actor->GetActorLocation(), GetHoverPoint()) <= Radius * Radius;
}

bool AEOMissionSite::IsOutsideArenaUnloadRadius(const AActor* Actor) const
{
	if (!Actor)
	{
		return true;
	}

	// Never narrower than the load radius, or the two could disagree about the
	// same position and the level would load and unload on alternate frames.
	const float Radius = GetHoverRadius() * FMath::Max(ArenaUnloadRadiusScale, ArenaLoadRadiusScale);
	return FVector::DistSquared(Actor->GetActorLocation(), GetHoverPoint()) > Radius * Radius;
}

void AEOMissionSite::SetArenaRequested(bool bRequested)
{
	if (!HasArena())
	{
		return;
	}

	if (!ArenaStreaming)
	{
		if (!bRequested)
		{
			return;
		}

		// Location only. A rotated level transform would turn the guard's
		// fixed-axis patrol offsets with it, and the site is placed axis-aligned.
		const FTransform Anchor(GetInsertionTransform().GetLocation());

		bool bLoaded = false;
		ArenaStreaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
			this, ArenaLevel, Anchor, bLoaded);

		if (!bLoaded || !ArenaStreaming)
		{
			UE_LOG(LogExecutiveOps, Error, TEXT("Mission site %s could not stream arena %s."),
				*GetName(), *ArenaLevel.ToString());
			ArenaStreaming = nullptr;
			return;
		}

		ArenaStreaming->OnLevelShown.AddDynamic(this, &AEOMissionSite::OnArenaShown);
		ArenaStreaming->OnLevelUnloaded.AddDynamic(this, &AEOMissionSite::OnArenaUnloaded);

		UE_LOG(LogExecutiveOps, Log, TEXT("Arena %s streaming in at %s."),
			*ArenaLevel.GetAssetName(), *Anchor.GetLocation().ToCompactString());
		return;
	}

	if (ArenaStreaming->ShouldBeLoaded() == bRequested)
	{
		return;
	}

	// The same streaming object is reused rather than removed and recreated: a
	// stream-out followed by a stream-in gives a fresh copy of the level either
	// way, and this keeps one handle to it for the life of the world.
	ArenaStreaming->SetShouldBeLoaded(bRequested);
	ArenaStreaming->SetShouldBeVisible(bRequested);

	UE_LOG(LogExecutiveOps, Log, TEXT("Arena %s %s."),
		*ArenaLevel.GetAssetName(), bRequested ? TEXT("requested") : TEXT("released"));
}

void AEOMissionSite::OnArenaShown()
{
	UE_LOG(LogExecutiveOps, Log, TEXT("Arena %s shown."), *ArenaLevel.GetAssetName());
}

void AEOMissionSite::OnArenaUnloaded()
{
	UE_LOG(LogExecutiveOps, Log, TEXT("Arena %s unloaded."), *ArenaLevel.GetAssetName());
}

void AEOMissionSite::OnMissionStateChanged_Implementation(EEOMissionState OldState, EEOMissionState NewState)
{
	// The marker is navigation aid only: once the player is on the ground it is
	// noise, so hide it for the duration of the ground phase.
	const bool bShowMarker =
		NewState == EEOMissionState::Inactive ||
		NewState == EEOMissionState::InFlight ||
		NewState == EEOMissionState::Deploying;

	if (Marker)
	{
		Marker->SetVisibility(bShowMarker);
	}
}
