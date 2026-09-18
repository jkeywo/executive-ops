#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EOGuardAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class AEOOperativeCharacter;
class UStateTreeAIComponent;
class UStateTree;

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

	/**
	 * True once a StateTree is actually running.
	 *
	 * The guard's C++ state machine stands down when this is true, so assigning
	 * a tree is the whole switch and clearing it is the whole revert. Until an
	 * asset is assigned the guard behaves exactly as it always has.
	 */
	bool IsRunningStateTree() const;

	/**
	 * Sends the brain back to the start.
	 *
	 * Resetting the pawn's fields is not enough on its own: the tree keeps
	 * whatever state it was in, and re-derives the pawn's state from it on the
	 * next tick. A guard put back on patrol would quietly re-alert and keep
	 * shooting.
	 */
	void RestartBrain();

	/**
	 * The tree that decides what this guard does. Optional.
	 *
	 * Left unset deliberately: the nodes in EOGuardStateTreeNodes.h exist and
	 * compile, but the graph wiring them together is an asset, and an asset is
	 * authored rather than generated. See Docs/adr/0005.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard|Brain")
	TObjectPtr<UStateTree> BrainTree;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Perception")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Perception")
	TObjectPtr<UAISenseConfig_Sight> Sight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guard|Brain")
	TObjectPtr<UStateTreeAIComponent> Brain;

private:
	/** Actors sight currently has. One entry, in practice. */
	TArray<TWeakObjectPtr<AActor>> SensedActors;

	mutable TWeakObjectPtr<AEOOperativeCharacter> CachedOperative;
};
