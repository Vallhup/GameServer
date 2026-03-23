#include "pch.h"
#include "AIExecutionSystem.h"

void AIExecutionSystem::Execute(const double dT)
{
	for (const auto& [entity, actionState, aiIntent, entityCommand, execState] :
		_runtime.GetECS().View<ActionState, AIIntent, EntityCommandFrame, AIExecutionState>())
	{
		execState.ClearFrameTransient();
		entityCommand.ClearFrameTransient();

		ExecuteMove(actionState, aiIntent, execState, entityCommand);
		ExecuteLook(aiIntent, entityCommand);
		ExecuteAction(actionState, aiIntent, execState, entityCommand);

		if (entityCommand.HasAnyCommand())
			entityCommand.source = CommandSource::AI;
	}
}

void AIExecutionSystem::ExecuteMove(
	const ActionState& actionState, 
	const AIIntent& aiIntent,
	const AIExecutionState& execState, 
	EntityCommandFrame& entityCommand)
{
	if (aiIntent.motion.hasMove)
	{
		if (CanWriteMove(actionState) && !execState.suppressMoveThisFrame)
		{
			XMVECTOR dirV = XMLoadFloat3(&aiIntent.motion.moveDir);
			if (TransformHelper::SafeNormalize3(dirV, dirV))
			{
				entityCommand.move.hasMove = true;
				entityCommand.move.moveRun = aiIntent.motion.moveRun;
				XMStoreFloat3(&entityCommand.move.moveDir, dirV);
			}

			else
			{
				entityCommand.move.Clear();
			}
		}

		else
		{
			entityCommand.move.Clear();
		}
	}

	else
	{
		entityCommand.move.Clear();
	}
}

void AIExecutionSystem::ExecuteLook(
	const AIIntent& aiIntent, 
	EntityCommandFrame& entityCommand)
{
	if (aiIntent.motion.hasLookTarget)
	{
		entityCommand.look.hasLook = true;
		entityCommand.look.target = aiIntent.motion.lookTarget;
	}

	else
	{
		entityCommand.look.Clear();
	}
}

void AIExecutionSystem::ExecuteAction(
	const ActionState& actionState, 
	const AIIntent& aiIntent, 
	AIExecutionState& execState, 
	EntityCommandFrame& entityCommand)
{
	if (aiIntent.action.hasAction)
	{
		if (aiIntent.action.sequence != 0 &&
			aiIntent.action.sequence == execState.lastConsumedActionSeq)
		{
			entityCommand.action.Clear();
			return;
		}


		if (CanWriteAction(actionState) && !execState.suppressActionThisFrame)
		{
			entityCommand.action.hasAction = true;
			entityCommand.action.actionType = aiIntent.action.actionType;
			entityCommand.action.attackType = aiIntent.action.attackType;
			entityCommand.action.sequence = aiIntent.action.sequence;

			if (aiIntent.action.sequence != 0)
				execState.lastConsumedActionSeq = aiIntent.action.sequence;
		}

		else
		{
			entityCommand.action.Clear();
		}
	}

	else
	{
		entityCommand.action.Clear();
	}
}

bool AIExecutionSystem::CanWriteMove(const ActionState& actionState)
{
}

bool AIExecutionSystem::CanWriteAction(const ActionState& actionState)
{
	return false;
}
