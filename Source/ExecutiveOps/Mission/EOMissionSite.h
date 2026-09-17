#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EOMissionParticipantInterface.h"
#include "EOMissionSite.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * A place in the city the player can fly a mission to.
 *
 * Owns the hover volume the aircraft has to be inside and stable within before it
 * can deploy. M2 uses it as a navigation target and a highlight; M3 uses the same
 * volume to gate the deployment sequence, and M7 to place the extraction.
 *
 * There is exactly one of these in the prototype. It is an actor rather than data
 * so that it can be dragged around the greybox map while tuning approach routes.
 */
UCLASS()
class EXECUTIVEOPS_API AEOMissionSite : public AActor, public IEOMissionParticipantInterface
{
	GENERATED_BODY()

public:
	AEOMissionSite();

	virtual void Tick(float DeltaSeconds) override;

	//~ IEOMissionParticipantInterface
	virtual void OnMissionStateChanged_Implementation(EEOMissionState OldState, EEOMissionState NewState) override;
	//~ End

	/** Name shown on the waypoint and in the mission list. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	FText GetDisplayName() const { return DisplayName; }

	/** Where the aircraft should sit to deploy. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	FVector GetHoverPoint() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetHoverRadius() const;

	/** True if the actor is inside the hover volume. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	bool IsWithinHoverVolume(const AActor* Actor) const;

	/** Where the operative is placed when it deploys. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	FTransform GetInsertionTransform() const;

protected:
	virtual void BeginPlay() override;

	/** The volume the aircraft has to be inside to deploy. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<USphereComponent> HoverVolume;

	/** Visible marker above the site so it reads from a distance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<UStaticMeshComponent> Marker;

	/** Ground position the operative starts from. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<USceneComponent> InsertionPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FText DisplayName = FText::FromString(TEXT("Test Site"));

	/** Marker bob and spin, so the site is findable in a grey city. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerSpinRate = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerBobHeight = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerBobRate = 1.2f;

private:
	float MarkerBaseZ = 0.f;
	float Elapsed = 0.f;
};
