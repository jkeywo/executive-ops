#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EOMissionParticipantInterface.h"
#include "EOMissionSite.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ULevelStreamingDynamic;

/**
 * A place in the city the player can fly a mission to.
 *
 * Owns the hover volume the aircraft has to be inside and stable within before it
 * can deploy. M2 uses it as a navigation target and a highlight; M3 uses the same
 * volume to gate the deployment sequence, and M7 to place the extraction.
 *
 * Also owns the arena: the ground level streamed into the flight map with its
 * origin on the insertion point while the aircraft is near. See ADR 0009.
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

	/** True when a level is assigned to stream in at this site. */
	UFUNCTION(BlueprintPure, Category = "Mission|Arena")
	bool HasArena() const { return !ArenaLevel.IsNull(); }

	/**
	 * True once the arena is loaded and visible, so the operative has something
	 * to land on. A site with no arena is always ready: the roof is the arena.
	 */
	UFUNCTION(BlueprintPure, Category = "Mission|Arena")
	bool IsArenaReady() const;

	/** Inside the radius at which the arena should be streamed in. */
	UFUNCTION(BlueprintPure, Category = "Mission|Arena")
	bool IsWithinArenaLoadRadius(const AActor* Actor) const;

	/** Beyond the radius at which the arena can be let go again. */
	UFUNCTION(BlueprintPure, Category = "Mission|Arena")
	bool IsOutsideArenaUnloadRadius(const AActor* Actor) const;

	/**
	 * Ask for the arena to be resident or not. The first request creates the
	 * level instance at the insertion point; later ones toggle the same streaming
	 * object, which is how the engine expects a runtime instance to be reused.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mission|Arena")
	void SetArenaRequested(bool bRequested);

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

	/**
	 * The ground level streamed in at this site, positioned so its origin sits
	 * on the insertion point. Authored with no lighting and no PlayerStart: it is
	 * a piece of the flight map, not a map of its own.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Arena")
	TSoftObjectPtr<UWorld> ArenaLevel;

	/** Load when the aircraft is within this many hover radii. 1 is the hover volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Arena", meta = (ClampMin = "0.1"))
	float ArenaLoadRadiusScale = 1.f;

	/**
	 * Unload only beyond this many hover radii. Wider than the load radius so the
	 * boundary cannot thrash, and because every stream-out is followed by a
	 * forced garbage collection.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Arena", meta = (ClampMin = "0.1"))
	float ArenaUnloadRadiusScale = 2.f;

	/** Marker bob and spin, so the site is findable in a grey city. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerSpinRate = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerBobHeight = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerBobRate = 1.2f;

private:
	UFUNCTION()
	void OnArenaShown();

	UFUNCTION()
	void OnArenaUnloaded();

	/** The runtime instance of ArenaLevel, once it has been requested. */
	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> ArenaStreaming;

	float MarkerBaseZ = 0.f;
	float Elapsed = 0.f;
};
