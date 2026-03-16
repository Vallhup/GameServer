#include "pch.h"
#include "AICommandBuildSystem.h"

#include "AI.h"
#include "Tags.h"
#include "Action.h"

AICommandBuildSystem::AICommandBuildSystem(WorldRuntime& rt, int p)
	: System(rt, p), _rng(std::random_device{}()), _uid(0, 99)
{
}

void AICommandBuildSystem::Execute(double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, actionState, aiState, sense, behavior, req, tuning, cmd] :
		ecs.View<ActionState, AIState, AISenseState, AIBehavior,
		AIActionRequestState, AICombatTuning, AICommand>(Exclude<DisconnectedTag>()))
	{
		BuildCommand(
			actionState,
			aiState,
			sense,
			behavior,
			req,
			tuning,
			cmd
		);
	}
}

void AICommandBuildSystem::BuildCommand(
	const ActionState& actionState, 
	const AIState& aiState, 
	const AISenseState& sense, 
	const AIBehavior& behavior,
	const AIActionRequestState& req, 
	const AICombatTuning& tuning, 
	AICommand& outCmd)
{
	const AICommand prev = outCmd;
	outCmd.Clear();

	switch (behavior.state) {
	case AIBehaviorState::Idle:
	case AIBehaviorState::AttackWindow:
	case AIBehaviorState::CommitAttack:
	case AIBehaviorState::Recover:
	{
		outCmd.type = AICommandType::Move;
		outCmd.moveMode = AIMoveMode::Hold;
		break;
	}

	case AIBehaviorState::Reposition:
	{
		if (!sense.hasTarget || sense.targetLost)
		{
			outCmd.type = AICommandType::Move;
			outCmd.moveMode = AIMoveMode::Hold;
			break;
		}

		if (behavior.stateTime < behavior.moveBurstDuration)
		{
			outCmd.type = AICommandType::Move;
			outCmd.moveMode = behavior.moveBurstRun ? AIMoveMode::Run : AIMoveMode::Walk;
			outCmd.moveDir = behavior.moveBurstDir;
		}
		else
		{
			outCmd.type = AICommandType::Move;
			outCmd.moveMode = AIMoveMode::Hold;
		}
		break;
	}

#ifdef _DEBUG
	default:
	{
		outCmd.type = AICommandType::Move;
		outCmd.moveMode = AIMoveMode::Hold;
		break;
	}
#endif
	}

	const bool changed =
		prev.type != outCmd.type ||
		prev.moveMode != outCmd.moveMode ||
		prev.attackType != outCmd.attackType ||
		prev.moveDir.x != outCmd.moveDir.x ||
		prev.moveDir.y != outCmd.moveDir.y ||
		prev.moveDir.z != outCmd.moveDir.z;

	outCmd.serial = changed ? (prev.serial + 1) : prev.serial;
}

XMFLOAT3 AICommandBuildSystem::MakeStrafeDir(bool strafeLeft, const XMFLOAT3& toTargetDir) const
{
	const XMFLOAT3 left{ -toTargetDir.z, 0.0f, toTargetDir.x };
	const XMFLOAT3 right{ toTargetDir.z, 0.0f, -toTargetDir.x };
	return strafeLeft ? left : right;
}

int AICommandBuildSystem::RandomPercent()
{
	return _uid(_rng);
}
