#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "EOGuardStateTreeNodes.generated.h"

class AEOGuardCharacter;

/**
 * The guard's behaviour, as nodes a StateTree can be built from.
 *
 * Each task owns one of the states the guard already had, and sets the pawn's
 * EEOGuardState on entry. That enum stays the published answer to "what is this
 * guard doing" - the HUD reads it, the encounter checks assert on it, and a
 * takedown is refused based on it - so the tree drives behaviour without
 * becoming the only place that knows what is happening.
 *
 * The pawn keeps the verbs: patrolling, pursuing and searching are still its
 * movement, and firing is still its weapon. Tasks decide when, the pawn decides
 * how. See Docs/adr/0005.
 */

USTRUCT()
struct FEOGuardNodeInstanceData
{
	GENERATED_BODY()

	/** Bind to the guard this tree is running for. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEOGuardCharacter> Guard = nullptr;
};

/** Walks the patrol, and reports itself finished the moment anything is noticed. */
USTRUCT(meta = (DisplayName = "EO Guard Patrol"))
struct EXECUTIVEOPS_API FEOGuardPatrolTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEOGuardNodeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/** Stands its ground and watches, while awareness climbs or decays. */
USTRUCT(meta = (DisplayName = "EO Guard Suspicious"))
struct EXECUTIVEOPS_API FEOGuardSuspiciousTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEOGuardNodeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/** Closes to its preferred range and shoots. */
USTRUCT(meta = (DisplayName = "EO Guard Engage"))
struct EXECUTIVEOPS_API FEOGuardEngageTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEOGuardNodeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/**
 * Sweeps the last known position for a while, then gives up.
 *
 * Succeeds when the search runs out, so the tree can transition back to patrol
 * on completion rather than needing a timer condition of its own.
 */
USTRUCT(meta = (DisplayName = "EO Guard Search"))
struct EXECUTIVEOPS_API FEOGuardSearchTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEOGuardNodeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FEOGuardDetectionConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEOGuardCharacter> Guard = nullptr;

	/** 0 is oblivious, 1 is certain. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", ClampMax = "1"))
	float Threshold = 1.f;
};

/** True while awareness is at or above the threshold. */
USTRUCT(meta = (DisplayName = "EO Guard Detection At Least"))
struct EXECUTIVEOPS_API FEOGuardDetectionCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	/**
	 * Flips the answer, so one node covers "is" and "is not".
	 *
	 * A per-condition flag rather than something the tree offers: StateTree has
	 * no invert of its own, and its own conditions each carry one. Needed here
	 * because a guard settling back down asks the opposite question to a guard
	 * noticing something.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;

	using FInstanceDataType = FEOGuardDetectionConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** True while the guard can currently see its target. */
USTRUCT(meta = (DisplayName = "EO Guard Sees Target"))
struct EXECUTIVEOPS_API FEOGuardSeesTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	/**
	 * Flips the answer, so one node covers "is" and "is not".
	 *
	 * A per-condition flag rather than something the tree offers: StateTree has
	 * no invert of its own, and its own conditions each carry one. Needed here
	 * because a guard settling back down asks the opposite question to a guard
	 * noticing something.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;

	using FInstanceDataType = FEOGuardNodeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT()
struct FEOGuardLostContactInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AEOGuardCharacter> Guard = nullptr;

	/** Seconds out of sight before contact counts as lost. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Seconds = 3.f;
};

/** True once the target has been out of sight for long enough. */
USTRUCT(meta = (DisplayName = "EO Guard Lost Contact"))
struct EXECUTIVEOPS_API FEOGuardLostContactCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	/**
	 * Flips the answer, so one node covers "is" and "is not".
	 *
	 * A per-condition flag rather than something the tree offers: StateTree has
	 * no invert of its own, and its own conditions each carry one. Needed here
	 * because a guard settling back down asks the opposite question to a guard
	 * noticing something.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;

	using FInstanceDataType = FEOGuardLostContactInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
