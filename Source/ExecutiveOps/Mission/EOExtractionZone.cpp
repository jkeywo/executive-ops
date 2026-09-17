#include "Mission/EOExtractionZone.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/EOPlayerController.h"
#include "ExecutiveOps.h"
#include "GameFramework/Pawn.h"
#include "Mission/EOMissionSubsystem.h"

AEOExtractionZone::AEOExtractionZone()
{
	PrimaryActorTick.bCanEverTick = true;

	Zone = CreateDefaultSubobject<USphereComponent>(TEXT("Zone"));
	Zone->InitSphereRadius(400.f);
	// Query only: the operative has to be able to run into it, not bounce off.
	Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zone->SetCollisionResponseToAllChannels(ECR_Overlap);
	RootComponent = Zone;

	Pad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pad"));
	Pad->SetupAttachment(RootComponent);
	Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pad->SetRelativeScale3D(FVector(7.f, 7.f, 0.1f));
	Pad->SetRelativeLocation(FVector(0.f, 0.f, -95.f));

	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(RootComponent);
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetRelativeLocation(FVector(0.f, 0.f, 320.f));
	Beacon->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
}

void AEOExtractionZone::BeginPlay()
{
	Super::BeginPlay();

	BeaconBaseZ = Beacon ? Beacon->GetRelativeLocation().Z : 0.f;

	// Dormant until the objective is done: an extraction point lit from the start
	// tells the player to leave before they have done anything.
	const bool bAvailable = Execute_IsExtractionAvailable(this);
	if (Pad)		{ Pad->SetVisibility(bAvailable); }
	if (Beacon)		{ Beacon->SetVisibility(bAvailable); }
}

void AEOExtractionZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Beacon || !Beacon->IsVisible())
	{
		return;
	}

	Elapsed += DeltaSeconds;
	Beacon->AddLocalRotation(FRotator(0.f, BeaconSpinRate * DeltaSeconds, 0.f));

	FVector Location = Beacon->GetRelativeLocation();
	Location.Z = BeaconBaseZ + FMath::Sin(Elapsed * 2.2f) * 40.f;
	Beacon->SetRelativeLocation(Location);
}

float AEOExtractionZone::GetZoneRadius() const
{
	return Zone ? Zone->GetScaledSphereRadius() : 0.f;
}

bool AEOExtractionZone::IsWithinZone(const AActor* Actor) const
{
	if (!Actor || !Zone)
	{
		return false;
	}

	// Distance rather than overlap events: the operative teleports during the
	// drop and traversals, and overlap bookkeeping across teleports is easy to
	// get subtly wrong.
	const float Radius = GetZoneRadius();
	return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= Radius * Radius;
}

bool AEOExtractionZone::IsExtractionAvailable_Implementation() const
{
	const UEOMissionSubsystem* Mission = GetWorld()
		? GetWorld()->GetSubsystem<UEOMissionSubsystem>() : nullptr;

	return Mission && Mission->GetMissionState() == EEOMissionState::ObjectiveComplete;
}

FTransform AEOExtractionZone::GetExtractionTransform_Implementation() const
{
	return GetActorTransform();
}

void AEOExtractionZone::RequestExtraction_Implementation(AActor* Requester)
{
	if (!Execute_IsExtractionAvailable(this))
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(Requester);
	AEOPlayerController* PlayerController = Pawn
		? Cast<AEOPlayerController>(Pawn->GetController()) : nullptr;

	if (!PlayerController)
	{
		UE_LOG(LogExecutiveOps, Warning, TEXT("Extraction requested by something with no player controller."));
		return;
	}

	PlayerController->RequestExtraction();
}

bool AEOExtractionZone::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor && Execute_IsExtractionAvailable(this);
}

bool AEOExtractionZone::Interact_Implementation(AActor* Interactor)
{
	if (!Execute_CanInteract(this, Interactor))
	{
		return false;
	}

	Execute_RequestExtraction(this, Interactor);
	return true;
}

FText AEOExtractionZone::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("EXTRACT [E]"));
}

float AEOExtractionZone::GetInteractionRange_Implementation() const
{
	// Standing anywhere on the pad is close enough. Hunting for a precise spot
	// while being shot at is not the tension this is meant to create.
	return GetZoneRadius();
}

void AEOExtractionZone::OnMissionStateChanged_Implementation(EEOMissionState OldState, EEOMissionState NewState)
{
	const bool bAvailable = (NewState == EEOMissionState::ObjectiveComplete);

	if (Pad)	{ Pad->SetVisibility(bAvailable); }
	if (Beacon)	{ Beacon->SetVisibility(bAvailable); }

	if (bAvailable)
	{
		UE_LOG(LogExecutiveOps, Log, TEXT("Extraction available."));
	}
}
