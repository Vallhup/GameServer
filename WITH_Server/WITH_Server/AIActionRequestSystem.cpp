#include "pch.h"
#include "AIActionRequestSystem.h"

#include "Event.h"

#include "AI.h"
#include "Tags.h"
#include "Action.h"
#include "AICommandBuildSystem.h"

AIActionRequestSystem::AIActionRequestSystem(WorldRuntime& rt, int p)
	: System(rt, p), _rng(std::random_device{}()), _uid(0, 99)
{
}

void AIActionRequestSystem::Execute(double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, actionState, aiState, sense, tuning, req] :
		ecs.View<ActionState, AIState, AISenseState, 
		AICombatTuning, AIActionRequestState>(Exclude<DisconnectedTag>()))
	{
		if (req.status == AIActionRequestStatus::None) continue;

		req.elapsed += static_cast<float>(dT);

		switch (req.status) {
		case AIActionRequestStatus::Requested:
		{
			ProcessAttackRequest(entity, actionState, aiState, sense, tuning, req);
			break;
		}
		case AIActionRequestStatus::Running:
		{
			if (req.requestIssued &&
				actionState.action != ActionType::Attack)
			{
				req.status = AIActionRequestStatus::Finished;
				req.acceptedByActionSystem = false;
				req.elapsed = 0.0;
			}
			break;
		}
		case AIActionRequestStatus::Finished:
		case AIActionRequestStatus::Rejected:
		default:
			break;
		}
	}
}

void AIActionRequestSystem::ProcessAttackRequest(
	Entity entity, 
	const ActionState& actionState, 
	AIState& aiState, 
	const AISenseState& sense, 
	const AICombatTuning& tuning, 
	AIActionRequestState& req)
{
	// 타겟이 사라졌으면 즉시 거절
	if (!sense.hasTarget || sense.targetLost)
	{
		req.status = AIActionRequestStatus::Rejected;
		req.acceptedByActionSystem = false;
		req.elapsed = 0.0;
		return;
	}

	// 아직 공격 타입이 안 정해졌으면 여기서 결정
	if (req.requestedAttack == AttackType::None)
	{
		req.requestedAttack = SelectAttack(entity, aiState, sense, tuning);

		if (req.requestedAttack == AttackType::None)
		{
			req.status = AIActionRequestStatus::Rejected;
			return;
		}
	}

	// 이미 실제 공격 액션에 들어갔으면 Running
	if (req.requestIssued &&
		actionState.action == ActionType::Attack)
	{
		req.status = AIActionRequestStatus::Running;
		req.acceptedByActionSystem = true;
		req.elapsed = 0.0;
		return;
	}

	// 아직 요청을 안 보냈으면 1회 발행
	if (!req.requestIssued)
	{
		ActionRequestEvent ev;
		ev.entity = entity;
		ev.actionType = ActionType::Attack;
		ev.attackType = req.requestedAttack;
		ev.reason = ActionRequestReason::FromAI;

		_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);

		req.requestIssued = true;
		aiState.lastAttack = req.requestedAttack;
		return;
	}

	// 요청은 보냈는데 일정 시간 안에 공격 액션 시작 안 되면 실패
	if (req.elapsed >= tuning.actionRequestTimeout)
	{
		req.status = AIActionRequestStatus::Rejected;
		req.acceptedByActionSystem = false;
		req.elapsed = 0.0;
	}
}

AttackType AIActionRequestSystem::SelectAttack(
	Entity self,
	const AIState& aiState,
	const AISenseState& sense,
	const AICombatTuning& tuning)
{
	const float distSq = sense.distSqToTarget;

	const float nearDistSq = tuning.nearCheckDistance * tuning.nearCheckDistance;
	const float midDistSq = tuning.midCheckDistance * tuning.midCheckDistance;
	const float farDistSq = tuning.farCheckDistance * tuning.farCheckDistance;

	// Meteor 조건
	if (sense.nearbyPlayerCount >= tuning.meteorMinTargets &&
		RandomPercent() < 25)
	{
		if (aiState.lastAttack != AttackType::Meteor)
			return AttackType::Meteor;
	}

	// 원거리
	if (distSq > farDistSq)
	{
		if (aiState.lastAttack != AttackType::MultiSlash)
			return AttackType::MultiSlash;
	}

	// 중거리
	if (distSq > midDistSq)
	{
		if (RandomPercent() < 50)
		{
			if (aiState.lastAttack != AttackType::DashSlash)
				return AttackType::DashSlash;
		}

		else
		{
			if (aiState.lastAttack != AttackType::JumpSlash)
				return AttackType::JumpSlash;
		}
	}

	// 근거리
	if (distSq > nearDistSq)
	{
		if (RandomPercent() < 50)
		{
			if (aiState.lastAttack != AttackType::Slash)
				return AttackType::Slash;

			else
				return AttackType::Thrust;
		}

		else
		{
			if (aiState.lastAttack != AttackType::Thrust)
				return AttackType::Thrust;

			else
				return AttackType::Slash;
		}
	}

	return AttackType::None;
}

int AIActionRequestSystem::RandomPercent()
{
	return _uid(_rng);
}
