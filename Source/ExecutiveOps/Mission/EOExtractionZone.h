#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EOExtractionInterface.h"
#include "Interfaces/EOMissionParticipantInterface.h"
#include "Mission/EOInteractableInterface.h"
#include "EOExtractionZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * Where the operative leaves from.
 *
 * Dormant until the objective is done, then it lights up and becomes the thing
 * the player is running for. M6 ends the mission on arrival; M7 replaces that
 * with the aircraft actually coming to get them.
 */
UCLASS()
class EXECUTIVEOPS_API AEOExtractionZone : public AActor,
	public IEOExtractionInterface, public IEOInteractableInterface,
	public IEOMissionParticipantInterface
{
	GENERATED_BODY()

public:
	AEOExtractionZone();

	virtual void Tick(float DeltaSeconds) override;

	//~ IEOExtractionInterface
	virtual bool IsExtractionAvailable_Implementation() const override;
	virtual void RequestExtraction_Implementation(AActor* Requester) override;
	virtual FTransform GetExtractionTransform_Implementation() const override;
	//~ End

	//~ IEOInteractableInterface
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual float GetInteractionRange_Implementation() const override;
	//~ End

	//~ IEOMissionParticipantInterface
	virtual void OnMissionStateChanged_Implementation(EEOMissionState OldState, EEOMissionState NewState) override;
	//~ End

	/** True if the actor is standing in the zone. */
	UFUNCTION(BlueprintPure, Category = "Extraction")
	bool IsWithinZone(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Extraction")
	float GetZoneRadius() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction")
	TObjectPtr<USphereComponent> Zone;

	/** Marks the pad on the ground. Only visible once extraction is live. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction")
	TObjectPtr<UStaticMeshComponent> Pad;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction")
	TObjectPtr<UStaticMeshComponent> Beacon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	float BeaconSpinRate = 120.f;

private:
	float BeaconBaseZ = 0.f;
	float Elapsed = 0.f;
};
