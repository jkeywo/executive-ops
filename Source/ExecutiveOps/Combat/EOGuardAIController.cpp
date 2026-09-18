#include "Combat/EOGuardAIController.h"

#include "Character/EOOperativeCharacter.h"
#include "Combat/EOGuardCharacter.h"

#include "EngineUtils.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Components/StateTreeAIComponent.h"
#include "StateTreeExecutionTypes.h"

AEOGuardAIController::AEOGuardAIController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception);

	Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));

	// No teams are configured, so affiliation would filter everything out if it
	// were left at its defaults. The guard has exactly one thing worth seeing and
	// decides for itself whether it counts.
	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = true;
	Sight->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*Sight);
	Perception->SetDominantSense(Sight->GetSenseImplementation());

	Perception->OnTargetPerceptionUpdated.AddDynamic(
		this, &AEOGuardAIController::HandlePerceptionUpdated);

	Brain = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("Brain"));
}

void AEOGuardAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AEOGuardCharacter* Guard = Cast<AEOGuardCharacter>(InPawn);
	if (!Guard || !Sight)
	{
		return;
	}

	// Taken from the pawn so a guard is still retuned in one place. The cone is
	// the same shape it was when this was hand-rolled: a half-angle either side
	// of where the guard is facing, out to its sight range.
	Sight->SightRadius = Guard->GetSightRange();
	Sight->PeripheralVisionAngleDegrees = Guard->GetSightHalfAngle();

	// A little further than it takes to acquire, or a target hovering at exactly
	// the edge flickers in and out and the detection ramp stutters with it.
	Sight->LoseSightRadius = Guard->GetSightRange() * 1.15f;

	Perception->ConfigureSense(*Sight);
	Perception->RequestStimuliListenerUpdate();

	// Only starts if an asset has been assigned. Without one the guard keeps its
	// C++ state machine, which is why this can land before the graph exists.
	if (Brain && BrainTree)
	{
		Brain->SetStateTree(BrainTree);
		Brain->StartLogic();
	}
}

bool AEOGuardAIController::IsRunningStateTree() const
{
	return Brain && BrainTree && Brain->IsRunning();
}

void AEOGuardAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		SensedActors.AddUnique(Actor);
	}
	else
	{
		SensedActors.Remove(Actor);
	}
}

bool AEOGuardAIController::IsPerceiving(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	for (const TWeakObjectPtr<AActor>& Weak : SensedActors)
	{
		if (Weak.Get() == Actor)
		{
			return true;
		}
	}

	return false;
}

AEOOperativeCharacter* AEOGuardAIController::GetOperative() const
{
	if (AEOOperativeCharacter* Cached = CachedOperative.Get())
	{
		return Cached;
	}

	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AEOOperativeCharacter> It(World); It; ++It)
		{
			CachedOperative = *It;
			break;
		}
	}

	return CachedOperative.Get();
}
