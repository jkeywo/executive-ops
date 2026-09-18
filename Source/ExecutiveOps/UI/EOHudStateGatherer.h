#pragma once

#include "CoreMinimal.h"
#include "Interfaces/EOMissionTypes.h"
#include "UI/EOHudState.h"
#include "UObject/Object.h"
#include "EOHudStateGatherer.generated.h"

class AEOPlayerController;
class AEOAircraftPawn;
class AEOExtractionZone;
class AEOGuardCharacter;
class UEOMissionSubsystem;

/**
 * Reads the world once and hands back an FEOHUDState.
 *
 * Pulled out of AEOPlayerHUD so that gathering is no longer something only an
 * AHUD can do. That matters for two reasons: the in-world panel wants the same
 * state as the viewport, and gathering can be exercised without a canvas.
 *
 * Continuous values are pulled - speed, health, detection, distances all change
 * every frame and polling them once here is cheaper than pushing each one. The
 * discrete change is bound instead: a mission transition happens eight times in
 * a run, and waiting up to a second for the actor scan to notice meant the
 * interface could lag a state change the player had just caused.
 */
UCLASS()
class EXECUTIVEOPS_API UEOHudStateGatherer : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Fills Out from the world. False when there is nothing to draw for - no
	 * controller, or no pawn yet - which callers treat as "skip this frame"
	 * rather than as an error.
	 */
	bool Gather(AEOPlayerController* Controller, FEOHUDState& Out);

	/** Forces the next Gather to rescan rather than reuse the cache. */
	void InvalidateActorCache() { LastActorScanTime = -BIG_NUMBER; }

	/** Seconds between full actor scans. Guards move; which guards exist rarely changes. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0"))
	float ActorScanInterval = 1.f;

private:
	/** Subscribes to the mission subsystem the first time it is seen. */
	void EnsureMissionBinding(UEOMissionSubsystem* Mission);

	UFUNCTION()
	void HandleMissionStateChanged(EEOMissionState OldState, EEOMissionState NewState);

	void RefreshActorCache(UWorld* World);

	TArray<TWeakObjectPtr<AEOGuardCharacter>> CachedGuards;
	TWeakObjectPtr<AEOAircraftPawn> CachedAircraft;
	TWeakObjectPtr<AEOExtractionZone> CachedExtraction;

	TWeakObjectPtr<UEOMissionSubsystem> BoundMission;

	float LastActorScanTime = -BIG_NUMBER;
};
