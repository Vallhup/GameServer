#include "pch.h"
#include "AIPerceptionSystem.h"

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta AIPerceptionSystem::kMeta =
	MakeSystemMeta<AIPerceptionSystem>("AIPerceptionSystem");

double AIPerceptionSystem::DistanceSqXZ(
	const XMFLOAT3& a,
	const XMFLOAT3& b) noexcept
{
	const double dx = static_cast<double>(a.x - b.x);
	const double dz = static_cast<double>(a.z - b.z);
	return dx * dx + dz * dz;
}

void AIPerceptionSystem::DirectionXZ(
	const XMFLOAT3& from,
	const XMFLOAT3& to,
	double& outX,
	double& outZ) noexcept
{
	const double dx = static_cast<double>(to.x - from.x);
	const double dz = static_cast<double>(to.z - from.z);
	const double len = std::sqrt(dx * dx + dz * dz);
	if (len < 1e-9)
	{
		outX = 0.0;
		outZ = 1.0;
		return;
	}
	outX = dx / len;
	outZ = dz / len;
}

double AIPerceptionSystem::ComputeScore(
	const PerceptionCandidate& candidate,
	const AIPerceptionTuningComp& tuning) const noexcept
{
	if (!candidate.inSightRange)
	{
		return std::numeric_limits<double>::lowest();
	}

	const double sightSq = tuning.sightRange * tuning.sightRange;
	const double safeSightSq = std::max(0.001, sightSq);
	double distanceScore = 1.0 - (candidate.distSq / safeSightSq);
	distanceScore = std::clamp(distanceScore, 0.0, 1.0);

	double score = distanceScore * 10.0;
	if (candidate.isCurrentTarget)  score += tuning.targetKeepBonus;
	if (candidate.isLastAttacker)   score += tuning.lastAttackerBonus;
	if (candidate.inFront)          score += tuning.frontBonus;
	return score;
}

AIPerceptionSystem::PerceptionCandidate AIPerceptionSystem::EvaluateCandidate(
	Entity self,
	const WorldTransformComp& selfTr,
	const WorldTransformComp& otherTr,
	const AIPerceptionTuningComp& tuning,
	const AIBlackboardComp& blackboard,
	Entity other) const noexcept
{
	(void)self;

	PerceptionCandidate out;
	out.entity = other;
	out.isCurrentTarget = (blackboard.currentTarget == other);
	out.isLastAttacker  = (blackboard.lastAttacker  == other);

	const double distSq = DistanceSqXZ(selfTr.position, otherTr.position);
	out.distSq = distSq;

	const double sightSq  = tuning.sightRange  * tuning.sightRange;
	const double attackSq = tuning.attackRange * tuning.attackRange;
	out.inSightRange  = (distSq <= sightSq);
	out.inAttackRange = (distSq <= attackSq);

	// 전방 벡터: yawRad 기준 (sin(yaw), cos(yaw))가 XZ forward
	const double fwdX = std::sin(static_cast<double>(selfTr.yawRad));
	const double fwdZ = std::cos(static_cast<double>(selfTr.yawRad));

	double toX = 0.0, toZ = 0.0;
	DirectionXZ(selfTr.position, otherTr.position, toX, toZ);

	out.forwardDot = fwdX * toX + fwdZ * toZ;
	out.inFront    = (out.forwardDot >= tuning.frontDotThreshold);

	// 시야 차단 없음 (TODO: 나중에 raycast 추가 가능)
	out.visible = out.inSightRange;
	out.score   = ComputeScore(out, tuning);
	return out;
}

void AIPerceptionSystem::BuildPerception(
	Entity self,
	const WorldTransformComp& selfTr,
	const AIPerceptionTuningComp& tuning,
	AIBlackboardComp& blackboard,
	AIPerceptionComp& cache,
	const ECSView& ecs,
	double dT) const
{
	cache = {};
	cache.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;

	const double leashSq = tuning.leashRange * tuning.leashRange;

	PerceptionCandidate best;
	PerceptionCandidate currentCand;
	bool foundAny = false;
	bool hasCurrentTargetCandidate = false;

	// 타겟 후보: PlayerControlIdentityComp 보유 엔티티
	for (const auto& [other, otherTr, _] :
		ecs.View<WorldTransformComp, PlayerControlIdentityComp>())
	{
		if (other == self) continue;

		const double distSq = DistanceSqXZ(selfTr.position, otherTr.position);
		if (distSq > leashSq) continue;

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

	// 후보 없음 → graceTime 후 타겟 초기화
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

	// 타겟 선택
	PerceptionCandidate finalCand;
	if (blackboard.forceRetarget || blackboard.currentTarget.IsNull())
	{
		finalCand = best;
	}
	else if (!hasCurrentTargetCandidate)
	{
		finalCand = best;
	}
	else
	{
		if (best.entity != blackboard.currentTarget &&
			best.score > currentCand.score + tuning.switchScoreMargin)
		{
			finalCand = best;
		}
		else
		{
			finalCand = currentCand;
		}
	}

	// 블랙보드 갱신
	blackboard.forceRetarget  = false;
	blackboard.currentTarget  = finalCand.entity;

	if (finalCand.visible)
	{
		blackboard.timeSinceCurrentTargetSeen = 0.0;
		const WorldTransformComp* targetTr =
			ecs.GetComponent<WorldTransformComp>(finalCand.entity);
		if (targetTr != nullptr)
		{
			blackboard.lastKnownTargetPosition = targetTr->position;
			blackboard.hasLastKnownTargetPosition = true;
		}
	}
	else
	{
		blackboard.timeSinceCurrentTargetSeen += dT;
	}

	// 캐시 갱신
	cache.hasTarget            = true;
	cache.selectedTarget       = finalCand.entity;
	cache.distanceToTargetSq   = finalCand.distSq;
	cache.distanceToTarget     = std::sqrt(finalCand.distSq);
	cache.targetForwardDot     = finalCand.forwardDot;
	cache.targetVisible        = finalCand.visible;
	cache.targetInSightRange   = finalCand.inSightRange;
	cache.targetInAttackRange  = finalCand.inAttackRange;
	cache.targetInFront        = finalCand.inFront;
	cache.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;
}

void AIPerceptionSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, _, selfTr, tuning, blackboard, cache] :
		ctx.ecs.View<
			AIControlledTag,
			WorldTransformComp,
			AIPerceptionTuningComp,
			AIBlackboardComp,
			AIPerceptionComp>())
	{
		BuildPerception(
			entity,
			selfTr,
			tuning,
			blackboard,
			cache,
			ctx.ecs,
			ctx.dtSec);
	}
}

const SystemMeta& AIPerceptionSystem::Meta() const
{
	return kMeta;
}
