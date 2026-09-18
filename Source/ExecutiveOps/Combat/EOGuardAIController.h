#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EOGuardAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class AEOOperativeCharacter;

/**
 * The guard's senses, and in time its brain.
 *
 * The guard used to find the player by iterating every actor in the world, once
 * per guard per frame, and then judging range, cone and line of sight by hand.
 * Sight is now a UAIPerceptionComponent, which is the engine's answer to the
 * same question and the one that scales past a single guard: the sense runs on
 * its own schedule, the operative registers itself as something worth seeing,
 * and nothing walks the actor list.
 *
 * Brain in the controller, body in the pawn. The guard keeps movement, animation
 * and health; what it knows lives here. See Docs/adr/0005.
 *
 * The cone is configured from the pawn's own tuning on possession rather than
 * duplicated here, so there is still one place to retune a guard.
 */
UCLASS()
class EXECUTIVEOPS_API AEOGuardAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEOGuardAIController();

	/** True when sight currently has the actor, by the engine's reckoning. */
	bool IsPerceiving(const AActor* Actor) const;

	/**
	 * The operative, resolved once and remembered.
	 *
	 * The guard needs a candidate for the close-range check below, which by
	 * definition happens when sight has not reported anything. Resolving it
	 * lazily costs one scan on the first frame that asks, rather than one per
	 * frame per guard.
	 */
	AEOOperativeCharacter* GetOperative() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Perception")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Perception")
	TObjectPtr<UAISenseConfig_Sight> Sight;

private:
	/** Actors sight currently has. One entry, in practice. */
	TArray<TWeakObjectPtr<AActor>> SensedActors;

	mutable TWeakObjectPtr<AEOOperativeCharacter> CachedOperative;
};
