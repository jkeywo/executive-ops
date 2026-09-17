#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/EOMissionTypes.h"
#include "EOMissionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEOMissionStateChanged, EEOMissionState, OldState, EEOMissionState, NewState);

/**
 * Single owner of mission state. Deliberately a linear state machine with no mission
 * definition, no objectives and no generation — those arrive in M6 and beyond.
 *
 * Transitions are validated so that an illegal request is logged and dropped rather
 * than silently corrupting state.
 */
UCLASS()
class EXECUTIVEOPS_API UEOMissionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FEOMissionStateChanged OnMissionStateChanged;

	UFUNCTION(BlueprintPure, Category = "Mission")
	EEOMissionState GetMissionState() const { return MissionState; }

	/** Inactive -> InFlight. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool StartMission();

	/** InFlight -> Deploying. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool BeginDeployment();

	/** Deploying -> OnGround. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool CompleteDeployment();

	/** OnGround -> ObjectiveComplete. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool CompleteObjective();

	/** ObjectiveComplete -> Extracting. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool BeginExtraction();

	/** Extracting -> Complete. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool CompleteMission();

	/** Any active state -> Failed. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool FailMission();

	/** Drop straight back to Inactive. Used by the reset command. */
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void ResetMission();

	/** Human-readable state, for the debug HUD. */
	UFUNCTION(BlueprintPure, Category = "Mission")
	FString GetMissionStateName() const;

private:
	bool TryTransition(EEOMissionState From, EEOMissionState To);
	void SetMissionState(EEOMissionState NewState);

	UPROPERTY(Transient)
	EEOMissionState MissionState = EEOMissionState::Inactive;
};
