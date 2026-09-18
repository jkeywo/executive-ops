#include "Combat/EOGuardStateTreeNodes.h"

#include "Combat/EOGuardCharacter.h"
#include "StateTreeExecutionContext.h"

namespace
{
	/**
	 * The guard a node is bound to.
	 *
	 * A node with nothing bound is an authoring mistake rather than a runtime
	 * state, so it fails its state instead of quietly doing nothing - a guard
	 * standing still for no visible reason is the hardest kind of bug to see.
	 */
	AEOGuardCharacter* GuardFrom(const FEOGuardNodeInstanceData& Data)
	{
		return Data.Guard;
	}
}

EStateTreeRunStatus FEOGuardPatrolTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	Guard->SetGuardState(EEOGuardState::Patrolling);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardPatrolTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	Guard->TickPatrolMovement(DeltaTime);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardSuspiciousTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	Guard->SetGuardState(EEOGuardState::Suspicious);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardSuspiciousTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// Deliberately still. Committing to a move on a half-sighting is what makes
	// a guard read as twitchy rather than watchful; the turn toward the noise is
	// the pawn's own business.
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardEngageTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	Guard->SetGuardState(EEOGuardState::Alerted);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardEngageTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	Guard->TickPursueMovement(DeltaTime);
	Guard->TickEngagement(DeltaTime);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardSearchTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	Guard->SetGuardState(EEOGuardState::Searching);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEOGuardSearchTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AEOGuardCharacter* Guard = GuardFrom(Data);
	if (!Guard)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Succeeding on expiry lets the tree go back to patrol on completion rather
	// than needing a timer condition that duplicates the countdown.
	return Guard->TickSearchMovement(DeltaTime)
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Running;
}

bool FEOGuardDetectionCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	return Data.Guard && Data.Guard->GetDetectionAlpha() >= Data.Threshold;
}

bool FEOGuardSeesTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	return Data.Guard && Data.Guard->IsTargetVisible();
}

bool FEOGuardLostContactCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	return Data.Guard
		&& !Data.Guard->IsTargetVisible()
		&& Data.Guard->GetTimeSinceSeen() >= Data.Seconds;
}
