#include "pch.h"
#include "AIPerceptionSystem.h"

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

const SystemMeta AIPerceptionSystem::kMeta =
	MakeSystemMeta<AIPerceptionSystem>("AIPerceptionSystem");

void AIPerceptionSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, _, selfTr, tuning, blackboard, perception] :
		ctx.ecs.View<
		AIControlledTag, WorldTransformComp, AIPerceptionTuningComp,
		AIBlackboardComp, AIPerceptionComp>())
	{
		BuildPerception(entity, selfTr, tuning, blackboard, perception, ctx);
	}
}

double AIPerceptionSystem::ComputeScore(
	const PerceptionCandidate& candidate,
	const AIPerceptionTuningComp& tuning) const noexcept
{
	// 보이지 않는 거리면 최하점
	if (!candidate.inSightRange)
	{
		return std::numeric_limits<double>::lowest();
	}

	// 자신의 시야 범위를 기준으로 현재 거리를 정규화하고
	// 0 ~ 10점 사이로 반영
	const double sightSq = tuning.sightRange * tuning.sightRange;
	const double safeSightSq = std::max(0.001, sightSq);

	double distanceScore = 1.0 - (candidate.distSq / safeSightSq);
	distanceScore = std::clamp(distanceScore, 0.0, 1.0);

	double score{ distanceScore * 10.0 };

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
	const WorldTransformComp& selfTr,
	const WorldTransformComp& otherTr,
	const AIPerceptionTuningComp& tuning,
	const AIBlackboardComp& blackboard,
	Entity other) const noexcept
{
	PerceptionCandidate out;
	out.entity = other;
	out.isCurrentTarget = (blackboard.currentTarget == other);
	out.isLastAttacker  = (blackboard.lastAttacker  == other);

	const double distSq = TransformHelper::DistanceSq(selfTr, otherTr);
	out.distSq = distSq;

	const double sightSq  = tuning.sightRange  * tuning.sightRange;
	const double attackSq = tuning.attackRange * tuning.attackRange;

	out.inSightRange  = (distSq <= sightSq);
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
	const WorldTransformComp& selfTr, 
	const AIPerceptionTuningComp& tuning, 
	AIBlackboardComp& blackboard, 
	AIPerceptionComp& perception, 
	SystemContext& sysCtx)
{
	ECSView& ecs = sysCtx.ecs;

	perception = {};
	perception.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;

	PerceptionCandidate best;
	PerceptionCandidate currentCand;
	bool foundAny = false;
	bool hasCurrentTargetCandidate = false;

	// 타겟 후보: PlayerControlIdentityComp 보유 엔티티
	for (const auto& [other, otherTr, _] :
		ecs.View<WorldTransformComp, PlayerControlIdentityComp>())
	{
		if (other == self) continue;

		const double distSq = TransformHelper::DistanceSq(selfTr.position, otherTr.position);
		const double leashSq = tuning.leashRange * tuning.leashRange;
		if (distSq > leashSq) continue;

		PerceptionCandidate cand =
			EvaluateCandidate(selfTr, otherTr, tuning, blackboard, other);

		if (!cand.inSightRange) continue;

		++perception.hostileInSightCount;

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
		blackboard.timeSinceCurrentTargetSeen += sysCtx.dtSec;
		if (blackboard.timeSinceCurrentTargetSeen > tuning.loseSightGraceTime)
		{
			blackboard.currentTarget = Entity::Null();
		}

		perception.hasTarget = false;
		perception.selectedTarget = Entity::Null();
		perception.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;
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
	blackboard.forceRetarget = false;
	blackboard.currentTarget = finalCand.entity;

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
		blackboard.timeSinceCurrentTargetSeen += sysCtx.dtSec;
	}

	// 캐시 갱신
	perception.hasTarget = true;
	perception.selectedTarget = finalCand.entity;
	perception.distanceToTargetSq = finalCand.distSq;
	perception.distanceToTarget = std::sqrt(finalCand.distSq);
	perception.targetForwardDot = finalCand.forwardDot;
	perception.targetVisible = finalCand.visible;
	perception.targetInSightRange = finalCand.inSightRange;
	perception.targetInAttackRange = finalCand.inAttackRange;
	perception.targetInFront = finalCand.inFront;
	perception.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;
}