#include "pch.h"
#include "AIPerceptionSystem.h"

void AIPerceptionSystem::Execute(const double dT)
{
	for (const auto& [entity, selfTr, tuning, blackboard, cache] :
		_runtime.GetECS().View<Transform,
		AIPerceptionTuning, AIBlackboard, AIPerceptionCache>())
	{
		BuildPerception(entity, selfTr, tuning, blackboard, cache, dT);
	}
}

bool AIPerceptionSystem::IsTargetEntityValid(Entity self, Entity other) const
{
	if (other == Entity::Null() || self == other)
		return false;

	ECS& ecs = _runtime.GetECS();

	if (!ecs.GetStorage<PlayerTag>().HasComponent(other))
		return false;

	if (ecs.GetStorage<DisconnectedTag>().HasComponent(other))
		return false;

	return true;
}

bool AIPerceptionSystem::IsWithinLeash(
	const Transform& selfTr, 
	const Transform& otherTr, 
	const AIPerceptionTuning& tuning) const
{
	const double distSq = TransformHelper::DistanceSq(selfTr, otherTr);
	const double leashSq = tuning.leashRange * tuning.leashRange;
	return distSq <= leashSq;
}

double AIPerceptionSystem::ComputeScore(
	const PerceptionCandidate& candidate, 
	const AIPerceptionTuning& tuning) const
{
	// 보이지 않는 거리면 최하점
	if (!candidate.inSightRange)
		return (std::numeric_limits<double>::min)();

	// 자신의 시야 범위를 기준으로 현재 거리를 정규화하고
	// 0 ~ 10점 사이로 반영
	const double sightSq = tuning.sightRange * tuning.sightRange;
	const double safeSightSq = std::max(0.001, sightSq);

	double distanceScore = 1.0 - (candidate.distSq / safeSightSq);
	distanceScore = std::clamp(distanceScore, 0.0, 1.0);

	double score{ 0.0 };
	score += distanceScore * 10.0;

	// 현재 Target 유지 보너스
	if (candidate.isCurrentTarget)
		score += tuning.targetKeepBonus;

	// 마지막 공격자 보너스
	if (candidate.isLastAttacker)
		score += tuning.lastAttackerBonus;

	// 정면 보너스
	if (candidate.inFront)
		score += tuning.frontBonus;

	return score;
}

AIPerceptionSystem::PerceptionCandidate AIPerceptionSystem::EvaluateCandidate(
	Entity self, 
	const Transform& selfTr, 
	const Transform& otherTr, 
	const AIPerceptionTuning& tuning,
	const AIBlackboard& blackboard, 
	Entity other) const
{
	PerceptionCandidate out;
	out.entity = other;

	out.isCurrentTarget = (blackboard.currentTarget == other);
	out.isLastAttacker = (blackboard.lastAttacker == other);

	const double distSq = TransformHelper::DistanceSq(selfTr, otherTr);
	out.distSq = distSq;

	const double sightSq = tuning.sightRange * tuning.sightRange;
	const double attackSq = tuning.attackRange * tuning.attackRange;

	out.inSightRange = (distSq <= sightSq);
	out.inAttackRange = (distSq <= attackSq);

	const XMVECTOR selfForward = TransformHelper::Forward(selfTr);
	const XMVECTOR toTarget = TransformHelper::Direction(selfTr, otherTr);

	out.forwardDot = TransformHelper::Dot(selfForward, toTarget);
	out.inFront = (out.forwardDot >= tuning.frontDotThreshold);

	// TEMP : 추후 벽 뒤 감지 못함 같은 별도 정책 추가 가능
	out.visible = out.inSightRange;

	out.score = ComputeScore(out, tuning);
	return out;
}

void AIPerceptionSystem::BuildPerception(
	Entity self,
	const Transform& selfTr,
	const AIPerceptionTuning& tuning,
	AIBlackboard& blackboard,
	AIPerceptionCache& cache,
	const double dT)
{
	ECS& ecs = _runtime.GetECS();

	cache = {};
	cache.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;

	PerceptionCandidate best;
	PerceptionCandidate currentCand;
	bool foundAny{ false };
	bool hasCurrentTargetCandidate{ false };

	for (const auto& [other, otherTr, _] :
		ecs.View<Transform, PlayerTag>(Exclude<DisconnectedTag>()))
	{
		if (other == self) continue;

		if (!IsWithinLeash(selfTr, otherTr, tuning)) continue;

		PerceptionCandidate cand =
			EvaluateCandidate(self, selfTr, otherTr, tuning, blackboard, other);

		if (!cand.inSightRange) continue;
		
		++cache.hostileInSightCount;

		if (cand.isCurrentTarget)
		{
			currentCand = cand;
			hasCurrentTargetCandidate = true;
		}

		if (!foundAny || cand.score > best.score)
		{
			best = cand;
			foundAny = true;
		}
	}

	// 후보를 찾지 못했으면 currentTarget reset
	if (!foundAny)
	{
		blackboard.timeSinceCurrentTargetSeen += dT;
		if (blackboard.timeSinceCurrentTargetSeen > tuning.loseSightGraceTime)
		{
			blackboard.currentTarget = Entity::Null();
		}

		cache.hasTarget = false;
		cache.selectedTarget = Entity::Null();
		cache.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;
		return;
	}

	// 현재 타겟, 강제 리타겟 정책 등 고려하여 최종 결정
	Entity finalTarget{ Entity::Null() };
	PerceptionCandidate finalCand;

	if (blackboard.forceRetarget || blackboard.currentTarget == Entity::Null())
	{
		finalTarget = best.entity;
		finalCand = best;
	}

	else if (!hasCurrentTargetCandidate)
	{
		finalTarget = best.entity;
		finalCand = best;
	}

	else
	{
		if (best.entity != blackboard.currentTarget &&
			best.score > currentCand.score + tuning.switchScoreMargin)
		{
			finalTarget = best.entity;
			finalCand = best;
		}

		else
		{
			finalTarget = currentCand.entity;
			finalCand = currentCand;
		}
	}

	// blackboard 갱신 (타겟 확정)
	blackboard.forceRetarget = false;
	blackboard.currentTarget = finalTarget;

	if (finalCand.visible)
	{
		blackboard.timeSinceCurrentTargetSeen = 0.0;

		const auto* targetTr =
			ecs.GetStorage<Transform>().GetComponent(finalTarget);

		if (targetTr)
		{
			blackboard.lastKnownTargetPosition = targetTr->position;
			blackboard.hasLastKnownTargetPosition = true;
		}
	}
		

	else
	{
		blackboard.timeSinceCurrentTargetSeen += dT;
	}
		
	// Cache 기록
	cache.hasTarget = true;
	cache.selectedTarget = finalTarget;
	cache.distanceToTargetSq = finalCand.distSq;
	cache.distanceToTarget = std::sqrt(finalCand.distSq);
	cache.targetForwardDot = finalCand.forwardDot;
	cache.targetVisible = finalCand.visible;
	cache.targetInSightRange = finalCand.inSightRange;
	cache.targetInAttackRange = finalCand.inAttackRange;
	cache.targetInFront = finalCand.inFront;
	cache.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;
}
