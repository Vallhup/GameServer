#include "pch.h"
#include "AITacticSystem.h"

#include "AI.h"
#include "Tags.h"
#include "Action.h"

AITacticSystem::AITacticSystem(WorldRuntime& rt, int p)
	: System(rt, p), _rng(std::random_device{}()), _uid(0, 99)
{
}

void AITacticSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, actionState, aiState, sense, think, behavior, req] :
		ecs.View<ActionState, AIState, AISenseState, AIThinkState,
		AIBehavior, AIActionRequestState>(Exclude<DisconnectedTag>()))
	{
		think.thinkAcc				+= dT;
		think.attackCooldownAcc		+= dT;
		think.retargetCooldownAcc	+= dT;
		behavior.stateTime			+= dT;
		req.elapsed					+= dT;

		if (think.thinkAcc < think.thinkInterval) continue;
		think.thinkAcc -= think.thinkInterval;

		UpdateTactic(actionState, aiState, sense, think, behavior, req);
	}
}

void AITacticSystem::UpdateTactic(
	const ActionState& actionState, 
	AIState& aiState,
	const AISenseState& sense, 
	AIThinkState& think, 
	AIBehavior& behavior, 
	AIActionRequestState& req)
{
	switch (behavior.state) {
	case AIBehaviorState::Idle:
	{
		if (sense.hasTarget && !sense.targetLost)
		{
			if (sense.targetTooFar || sense.targetTooClose)
				EnterReposition(aiState, sense, behavior);
			else
				behavior.ChangeBehavior(AIBehaviorState::AttackWindow, RandRange(0.12, 0.28));
		}
		return;
	}

	case AIBehaviorState::Reposition:
	{
		if (!sense.hasTarget || sense.targetLost)
		{
			req.Clear();
			behavior.ChangeBehavior(AIBehaviorState::Idle, 0.0);
			return;
		}

		// burst가 끝났으면 잠깐 멈추는 공격 대기 상태로 감
		if (behavior.stateTime >= behavior.moveBurstDuration)
		{
			behavior.ChangeBehavior(AIBehaviorState::AttackWindow, RandRange(0.10, 0.25));
			return;
		}

		return;
	}

	case AIBehaviorState::AttackWindow:
	{
		if (!sense.hasTarget || sense.targetLost)
		{
			req.Clear();
			behavior.ChangeBehavior(AIBehaviorState::Idle, 0.0);
			return;
		}

		// 거리 틀어졌으면 다시 짧게만 보정
		if (sense.targetTooFar || sense.targetTooClose)
		{
			EnterReposition(aiState, sense, behavior);
			return;
		}

		const bool canAttack =
			think.attackCooldownAcc >= think.attackCooldown &&
			sense.targetInAttackRange &&
			req.status == AIActionRequestStatus::None;

		if (canAttack)
		{
			req.requestId += 1;
			req.status = AIActionRequestStatus::Requested;
			req.requestedAction = ActionType::Attack;
			req.requestedAttack = AttackType::None;
			req.elapsed = 0.0f;

			behavior.ChangeBehavior(AIBehaviorState::CommitAttack, 0.12);
			return;
		}

		return;
	}

	case AIBehaviorState::CommitAttack:
	{
		if (!sense.hasTarget || sense.targetLost)
		{
			req.Clear();
			behavior.ChangeBehavior(AIBehaviorState::Idle, 0.0);
			return;
		}

		if (req.status == AIActionRequestStatus::Running ||
			actionState.action == ActionType::Attack)
		{
			think.attackCooldownAcc = 0.0f;
			behavior.ChangeBehavior(AIBehaviorState::Recover, RandRange(0.18, 0.35));
			return;
		}

		if (req.status == AIActionRequestStatus::Rejected ||
			behavior.stateTime >= behavior.minStateDuration)
		{
			req.Clear();

			// 공격 요청이 실패했어도 다시 오래 움직이지 말고
			// 거리 안 맞으면 짧게 보정, 맞으면 대기
			if (sense.targetTooFar || sense.targetTooClose)
				EnterReposition(aiState, sense, behavior);
			else
				behavior.ChangeBehavior(AIBehaviorState::AttackWindow, RandRange(0.08, 0.18));

			return;
		}

		return;
	}

	case AIBehaviorState::Recover:
	{
		if (!sense.hasTarget || sense.targetLost)
		{
			req.Clear();
			behavior.ChangeBehavior(AIBehaviorState::Idle, 0.0);
			return;
		}

		const bool recovered =
			actionState.action != ActionType::Attack &&
			(req.status == AIActionRequestStatus::Finished ||
				req.status == AIActionRequestStatus::None);

		if (behavior.stateTime >= behavior.minStateDuration && recovered)
		{
			req.Clear();

			// 쉬는 타이밍에는 대부분 멈추고, 거리 틀어진 경우에만 짧게 보정
			if (sense.targetTooFar || sense.targetTooClose)
				EnterReposition(aiState, sense, behavior);
			else
				behavior.ChangeBehavior(AIBehaviorState::AttackWindow, RandRange(0.12, 0.24));

			return;
		}

		return;
	}
#ifdef _DEBUG
	default:
	{
		std::cout << "[AITacticSystem] Unknown tactic state: "
			<< static_cast<int>(behavior.state) << "\n";
		return;
	}
#endif
	}
}

void AITacticSystem::EnterReposition(
	AIState& aiState,
	const AISenseState& sense,
	AIBehavior& behavior)
{
	behavior.ChangeBehavior(AIBehaviorState::Reposition, 0.0);

	// 너무 멀면 조금 더 길게 접근
	if (sense.targetTooFar)
	{
		behavior.moveBurstDir = sense.toTargetDir;
		behavior.moveBurstRun = true;

		// 거리 크기에 비례하되 상한/하한은 제한
		const double extra = std::clamp(
			(sense.distToTarget - 1.5) * 0.12,
			0.0, 0.18);

		behavior.moveBurstDuration = 0.14 + extra;
		return;
	}

	// 너무 가까우면 짧게 후퇴
	if (sense.targetTooClose)
	{
		behavior.moveBurstDir = sense.awayFromTargetDir;
		behavior.moveBurstRun = false;
		behavior.moveBurstDuration = RandRange(0.10, 0.18);
		return;
	}

	// 적정 거리대인데도 각만 살짝 틀고 싶다면 매우 짧게 측면 이동
	if (RandomPercent() < 25)
	{
		behavior.moveBurstDir = MakeStrafeDir(aiState.strafeLeft, sense.toTargetDir);
		behavior.moveBurstRun = false;
		behavior.moveBurstDuration = RandRange(0.08, 0.14);

		aiState.strafeLeft = !aiState.strafeLeft;
		return;
	}

	// 기본은 정지 대기
	behavior.ChangeBehavior(AIBehaviorState::AttackWindow, RandRange(0.10, 0.20));
}

XMFLOAT3 AITacticSystem::MakeStrafeDir(bool strafeLeft, const XMFLOAT3& toTargetDir) const
{
	const XMFLOAT3 left{ -toTargetDir.z, 0.0f, toTargetDir.x };
	const XMFLOAT3 right{ toTargetDir.z, 0.0f, -toTargetDir.x };
	return strafeLeft ? left : right;
}

int AITacticSystem::RandomPercent()
{
	return _uid(_rng);
}

double AITacticSystem::RandRange(double minV, double maxV)
{
	const double t = static_cast<double>(_uid(_rng)) / 99.0;
	return minV + (maxV - minV) * t;
}
