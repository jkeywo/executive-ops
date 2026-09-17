#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EOMissionParticipantInterface.h"
#include "Mission/EOInteractableInterface.h"
#include "EOObjectiveTerminal.generated.h"

class UStaticMeshComponent;

/**
 * The objective: walk up to it and press interact.
 *
 * Deliberately trivial, as M6 asks. The mission exists to prove the loop holds
 * together - insert, cross the space, deal with the guard, do the thing, leave -
 * not to prove anything about objective mechanics. A minigame here would only
 * hide whether the loop works.
 */
UCLASS()
class EXECUTIVEOPS_API AEOObjectiveTerminal : public AActor,
	public IEOInteractableInterface, public IEOMissionParticipantInterface
{
	GENERATED_BODY()

public:
	AEOObjectiveTerminal();

	virtual void Tick(float DeltaSeconds) override;

	//~ IEOInteractableInterface
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual float GetInteractionRange_Implementation() const override { return InteractionRange; }
	//~ End

	//~ IEOMissionParticipantInterface
	virtual void OnMissionStateChanged_Implementation(EEOMissionState OldState, EEOMissionState NewState) override;
	//~ End

	UFUNCTION(BlueprintPure, Category = "Objective")
	bool IsComplete() const { return bComplete; }

	/** Used by the reset command so the mission can be run again. */
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void ResetObjective();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Floats above the terminal so it can be found across a greybox arena. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
	TObjectPtr<UStaticMeshComponent> Beacon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float InteractionRange = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText DisplayName = FText::FromString(TEXT("Terminal"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	float BeaconSpinRate = 90.f;

private:
	bool bComplete = false;
	float BeaconBaseZ = 0.f;
	float Elapsed = 0.f;
};
