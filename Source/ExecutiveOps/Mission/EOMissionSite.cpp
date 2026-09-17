#include "Mission/EOMissionSite.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
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
