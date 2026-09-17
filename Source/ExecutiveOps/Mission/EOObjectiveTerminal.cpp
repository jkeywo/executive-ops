#include "Mission/EOObjectiveTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "ExecutiveOps.h"
#include "Mission/EOMissionSubsystem.h"

AEOObjectiveTerminal::AEOObjectiveTerminal()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = Body;

	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(RootComponent);
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	Beacon->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
}

void AEOObjectiveTerminal::BeginPlay()
{
	Super::BeginPlay();
	BeaconBaseZ = Beacon ? Beacon->GetRelativeLocation().Z : 0.f;
}

void AEOObjectiveTerminal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Beacon)
	{
		return;
	}

	Elapsed += DeltaSeconds;
	Beacon->AddLocalRotation(FRotator(0.f, BeaconSpinRate * DeltaSeconds, 0.f));

	FVector Location = Beacon->GetRelativeLocation();
	Location.Z = BeaconBaseZ + FMath::Sin(Elapsed * 1.6f) * 25.f;
	Beacon->SetRelativeLocation(Location);
}

bool AEOObjectiveTerminal::CanInteract_Implementation(AActor* Interactor) const
{
	if (bComplete || !Interactor)
	{
		return false;
	}

	// Only while the mission is actually underway on the ground. Interacting
	// during the drop, or after extraction has been called, means nothing.
	const UEOMissionSubsystem* Mission = GetWorld()
		? GetWorld()->GetSubsystem<UEOMissionSubsystem>() : nullptr;

	return Mission && Mission->GetMissionState() == EEOMissionState::OnGround;
}

bool AEOObjectiveTerminal::Interact_Implementation(AActor* Interactor)
{
	if (!Execute_CanInteract(this, Interactor))
	{
		return false;
	}

	UEOMissionSubsystem* Mission = GetWorld()->GetSubsystem<UEOMissionSubsystem>();
	if (!Mission)
	{
		return false;
	}

	// Set before the transition: CompleteObjective broadcasts synchronously, and
	// the handler reads bComplete to decide whether the beacon is still wanted.
	bComplete = true;

	if (!Mission->CompleteObjective())
	{
		bComplete = false;
		return false;
	}

	if (Beacon)
	{
		Beacon->SetVisibility(false);
	}

	UE_LOG(LogExecutiveOps, Log, TEXT("Objective complete: %s"), *DisplayName.ToString());
	return true;
}

FText AEOObjectiveTerminal::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("DOWNLOAD [E]"));
}

void AEOObjectiveTerminal::OnMissionStateChanged_Implementation(EEOMissionState OldState, EEOMissionState NewState)
{
	// Once done, the beacon is just noise pointing at somewhere the player has
	// already been.
	if (Beacon)
	{
		Beacon->SetVisibility(!bComplete);
	}
}

void AEOObjectiveTerminal::ResetObjective()
{
	bComplete = false;
	if (Beacon)
	{
		Beacon->SetVisibility(true);
	}
}
