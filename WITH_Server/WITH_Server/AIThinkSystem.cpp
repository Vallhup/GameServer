#include "pch.h"
#include "AIThinkSystem.h"
#include "Framework.h"

#include "AI.h"
#include "Tags.h"
#include "Action.h"

AIThinkSystem::AIThinkSystem(WorldRuntime& rt, int p)
	: System(rt, p), _rng(std::random_device{}()), _uid(0, 99)
{
}

void AIThinkSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, actionState, aiState,
		aiThinkState, aiIntent, aiCombatTuning, transform] :
		ecs.View<ActionState, AIState, AIThinkState,
		AIIntent, AICombatTuning, Transform>())
	{
		aiThinkState.thinkAcc				+= dT;
		aiThinkState.attackCooldownAcc		+= dT;
		aiThinkState.retargetCooldownAcc	+= dT;

		if (aiThinkState.attackPending)
		{
			if (actionState.action != ActionType::Attack)
			{
				aiThinkState.attackPending = false;
			}
		}

		if (actionState.action != ActionType::None &&
			actionState.action != ActionType::Hit) continue;

		if (aiThinkState.thinkAcc < aiThinkState.thinkInterval)
			continue;

		aiThinkState.thinkAcc -= aiThinkState.thinkInterval;

		aiIntent.Clear();

		if (!EnsureTarget(entity, aiState, transform)) continue;

		const auto* targetTrans = 
			ecs.GetStorage<Transform>().GetComponent(aiState.target);

		if (!targetTrans)
		{
			aiState.target = {};
			aiState.patternsOnTarget = 0;
			continue;
		}

		DecideIntent(
			entity, 
			aiState, 
			aiThinkState, 
			transform, 
			*targetTrans, 
			aiCombatTuning, 
			aiIntent
		);
	}
}

bool AIThinkSystem::EnsureTarget(Entity self, AIState& aiState, const Transform& selfTrans)
{
	// 마지막 공격자 우선
	if (IsValidTarget(aiState.lastAttacker))
	{
		aiState.target = aiState.lastAttacker;
		return true;
	}

	// 현재 타겟 유지
	if (IsValidTarget(aiState.target))
	{
		return true;
	}

	// 새 타겟 탐색 (가장 가까운 Player)
	Entity nextTarget = FindClosestPlayer(self, selfTrans);
	if (nextTarget.IsNull())
	{
		aiState.target = {};
		aiState.patternsOnTarget = 0;
		return false;
	}

	aiState.target = nextTarget;
	aiState.patternsOnTarget = 0;
	return true;
}

bool AIThinkSystem::IsValidTarget(Entity target) const
{
	if (target.IsNull())
		return false;

	ECS& ecs = _runtime.GetECS();
	const bool isValidTarget =
		!ecs.GetStorage<DisconnectedTag>().HasComponent(target) &&
		 ecs.GetStorage<PlayerTag>().HasComponent(target) &&
		 ecs.GetStorage<Transform>().HasComponent(target);

	return isValidTarget;
}

Entity AIThinkSystem::FindClosestPlayer(Entity self, const Transform& selfTrans)
{
	ECS& ecs = _runtime.GetECS();

	Entity best;
	double bestDistSq{ std::numeric_limits<double>::max() };

	for (const auto& [entity, transform, playerTag] :
		ecs.View<Transform, PlayerTag>(Exclude<DisconnectedTag>()))
	{
		if (entity == self) continue;

		const double distSq = TransformHelper::DistanceSq(transform, selfTrans);
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = entity;
		}
	}

	return best;
}

void AIThinkSystem::DecideIntent(
	Entity self, 
	AIState& aiState, 
	AIThinkState& thinkState, 
	const Transform& selfTrans, 
	const Transform& targetTrans, 
	const AICombatTuning& tuning, 
	AIIntent& outIntent)
{
	const double distSq = TransformHelper::DistanceSq(selfTrans, targetTrans);

	const double preferredMinSq = tuning.preferredMinDistance * tuning.preferredMinDistance;
	const double preferredMaxSq = tuning.preferredMaxDistance * tuning.preferredMaxDistance;
	const double attackDistSq	= tuning.attackDistance * tuning.attackDistance;
	const double chaseDistSq	= tuning.chaseDistance * tuning.chaseDistance;
	const double loseTargetSq	= tuning.loseTargetDistance * tuning.loseTargetDistance;

	// Target Lose
	if (distSq > loseTargetSq)
	{
		aiState.target = {};
		aiState.patternsOnTarget = 0;
		aiState.hasTarget = false;

		outIntent.type = AIIntentType::Idle;
		return;
	}

	// 너무 멀면 추적
	if (distSq > chaseDistSq)
	{
		outIntent.type		= AIIntentType::Chase;
		outIntent.moveDir	= MakeMoveDir(selfTrans, targetTrans);
		outIntent.isRun		= true;
		return;
	}

	// 공격 가능 + 쿨다운 완료
	if (distSq <= attackDistSq &&
		thinkState.attackCooldownAcc >= thinkState.attackCooldown)
	{
		AttackType nextAttackType =
			SelectAttack(self, aiState, selfTrans, targetTrans, tuning, distSq);

		if (nextAttackType != AttackType::None)
		{
			outIntent.type			= AIIntentType::Attack;
			outIntent.attackType	= nextAttackType;
			outIntent.moveDir		= { 0.0f, 0.0f, 0.0f };
			outIntent.isRun			= false;
			return;
		}
	}

	// 너무 가까우면 후퇴
	if (distSq < preferredMinSq)
	{
		outIntent.type		= AIIntentType::Retreate;
		outIntent.moveDir	= MakeRetreatDir(selfTrans, targetTrans);
		outIntent.isRun		= false;
		return;
	}

	// 적정 범위 밖이면 접근
	if (distSq > preferredMaxSq)
	{
		outIntent.type		= AIIntentType::Chase;
		outIntent.moveDir	= MakeMoveDir(selfTrans, targetTrans);
		outIntent.isRun		= true;
		return;
	}

	// 적정 거리면 횡이동
	outIntent.type = AIIntentType::Strafe;
	outIntent.moveDir = MakeStrafeDir(aiState, selfTrans, targetTrans);
	outIntent.isRun = false;
}

AttackType AIThinkSystem::SelectAttack(
	Entity self, 
	AIState& aiState, 
	const Transform& selfTrans, 
	const Transform& targetTrans, 
	const AICombatTuning& tuning, 
	double distSq)
{
	const double nearDistSq{ 1.0 * 1.0 };
	const double midDistSq{ 10.0 * 10.0 };
	const double farDistSq{ 15.0 * 15.0 };

	const double meteorDistSq{ 5.0 * 5.0 };
	const int meteorCount = CountPlayersInRange(self, selfTrans, nearDistSq);

	// Meteor 조건
	if (meteorCount >= tuning.meteorMinTargets &&
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

int AIThinkSystem::CountPlayersInRange(Entity self, const Transform& center, double rangeSq) const
{
	ECS& ecs = _runtime.GetECS();

	int count{ 0 };
	for (const auto& [entity, playerTag, transform] :
		ecs.View<PlayerTag, Transform>(Exclude<DisconnectedTag>()))
	{
		if (entity != self) 
		{
			const double distSq = TransformHelper::DistanceSq(transform, center);
			if (distSq <= rangeSq) ++count;
		}
	}

	return count;
}

XMFLOAT3 AIThinkSystem::MakeMoveDir(const Transform& from, const Transform& to) 
{
	XMFLOAT3 dir
	{
		to.position.x - from.position.x,
		0.0f,
		to.position.z - from.position.z
	};
	XMVECTOR dirVec = XMLoadFloat3(&dir);
	TransformHelper::SafeNormalize3(dirVec, dirVec);
	XMStoreFloat3(&dir, dirVec);

	return dir;
}

XMFLOAT3 AIThinkSystem::MakeRetreatDir(const Transform& selfTrans, const Transform& targetTrans) 
{
	XMFLOAT3 dir
	{
		selfTrans.position.x - targetTrans.position.x,
		0.0f,
		selfTrans.position.z - targetTrans.position.z
	};
	XMVECTOR dirVec = XMLoadFloat3(&dir);
	TransformHelper::SafeNormalize3(dirVec, dirVec);
	XMStoreFloat3(&dir, dirVec);

	return dir;
}

XMFLOAT3 AIThinkSystem::MakeStrafeDir(AIState& aiState, const Transform& selfTrans, const Transform& targetTrans) 
{
	XMFLOAT3 toTarget = MakeMoveDir(selfTrans, targetTrans);

	XMFLOAT3 left{ -toTarget.z, 0.0f, toTarget.x };
	XMFLOAT3 right{ toTarget.z, 0.0f, -toTarget.x };

	if (RandomPercent() < 20)
		aiState.strafeLeft = !aiState.strafeLeft;

	return aiState.strafeLeft ? left : right;
}

int AIThinkSystem::RandomPercent() 
{
	return _uid(_rng);
}