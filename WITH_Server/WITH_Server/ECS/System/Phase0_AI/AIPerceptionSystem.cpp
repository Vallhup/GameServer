#include "pch.h"
#include "AIPerceptionSystem.h"

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 6> kAIPerceptionAccesses{
		ReadImmediate(ComponentRes<AIControlledTag>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AIPerceptionTuningComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		WriteImmediate(ComponentRes<AIBlackboardComp>()),
		WriteImmediate(ComponentRes<AIPerceptionComp>()),
	};

	static double ClampDouble(double value, double minValue, double maxValue) noexcept
	{
		return std::clamp(value, minValue, maxValue);
	}

	static double DistanceSqXZ(const XMFLOAT3& lhs, const XMFLOAT3& rhs) noexcept
	{
		const double dx = lhs.x - rhs.x;
		const double dz = lhs.z - rhs.z;
		return dx * dx + dz * dz;
	}

	static void ClearPerceptionTarget(AIPerceptionComp& perception)
	{
		perception.hasTarget = false;
		perception.selectedTarget = Entity::Null();
		perception.distanceToTarget = std::numeric_limits<double>::max();
		perception.distanceToTargetSq = std::numeric_limits<double>::max();
		perception.targetForwardDot = std::numeric_limits<double>::lowest();
		perception.targetVisible = false;
		perception.targetInSightRange = false;
		perception.targetInAttackRange = false;
		perception.targetInFront = false;
	}

	static void EnterReturnHome(
		AIBlackboardComp& blackboard,
		AIPerceptionComp& perception)
	{
		blackboard.currentTarget = Entity::Null();
		blackboard.forceRetarget = false;
		blackboard.returningHome = true;
		blackboard.returnHomeLockoutAcc = 0.0;
		blackboard.leashGauge = 0.0;
		ClearPerceptionTarget(perception);
	}
}

const SystemMeta AIPerceptionSystem::kMeta =
	SystemMeta{
		SysTag<AIPerceptionSystem>(),
		"AIPerceptionSystem",
		kAIPerceptionAccesses,
		kNoDeps,
		kNoDeps
	};

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
	if (!candidate.inSightRange && !candidate.isCurrentTarget)
	{
		return std::numeric_limits<double>::lowest();
	}

	const double sightSq = tuning.sightRange * tuning.sightRange;
	const double safeSightSq = std::max(0.001, sightSq);

	double distanceScore = 1.0 - (candidate.distSq / safeSightSq);
	distanceScore = std::clamp(distanceScore, 0.0, 1.0);

	double score{ distanceScore * 10.0 };
	if (candidate.isCurrentTarget)
		score += tuning.targetKeepBonus;
	if (candidate.isLastAttacker)
		score += tuning.lastAttackerBonus;
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

	if (!blackboard.hasHomePosition)
	{
		blackboard.homePosition = selfTr.position;
		blackboard.hasHomePosition = true;
	}

	if (blackboard.returnHomeLockoutAcc < tuning.returnHomeReaggroLockSec)
	{
		blackboard.returnHomeLockoutAcc += sysCtx.dtSec;
	}

	PerceptionCandidate best;
	PerceptionCandidate currentCand;
	bool foundAny = false;
	bool hasCurrentTargetCandidate = false;

	const bool reaggroLocked =
		blackboard.returningHome ||
		blackboard.returnHomeLockoutAcc < tuning.returnHomeReaggroLockSec;
	const double aggroSq = tuning.sightRange * tuning.sightRange;
	const double softLeashSq = tuning.leashRange * tuning.leashRange;
	const double hardLeashSq = tuning.hardLeashRange * tuning.hardLeashRange;

	for (const auto& [other, otherTr, __] :
		ecs.View<WorldTransformComp, SpawnTypeComp>())
	{
		if (other == self)
			continue;
		if (IsSameFaction(ecs, self, other))
			continue;

		const double homeDistSq =
			DistanceSqXZ(blackboard.homePosition, otherTr.position);
		const bool isCurrentTarget = (blackboard.currentTarget == other);
		const bool inAggroRange = homeDistSq <= aggroSq;
		const bool inHardLeashRange = homeDistSq <= hardLeashSq;
		const bool canAcquire = !reaggroLocked && inAggroRange;
		const bool canKeepCurrent = isCurrentTarget;

		if (!canAcquire && !canKeepCurrent)
			continue;

		PerceptionCandidate cand =
			EvaluateCandidate(selfTr, otherTr, tuning, blackboard, other);
		cand.inSightRange = inAggroRange;
		cand.visible = isCurrentTarget ? inHardLeashRange : inAggroRange;
		cand.score = ComputeScore(cand, tuning);

		if (inAggroRange)
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

	if (!foundAny)
	{
		blackboard.timeSinceCurrentTargetSeen += sysCtx.dtSec;
		if (blackboard.timeSinceCurrentTargetSeen > tuning.loseSightGraceTime)
		{
			if (!blackboard.currentTarget.IsNull())
			{
				EnterReturnHome(blackboard, perception);
			}
			else if (!blackboard.returningHome)
			{
				blackboard.leashGauge = ClampDouble(
					blackboard.leashGauge + tuning.leashRecoverPerSec * sysCtx.dtSec,
					0.0,
					tuning.leashGaugeMax);
			}
		}

		ClearPerceptionTarget(perception);
		perception.timeSinceTargetLastSeen = blackboard.timeSinceCurrentTargetSeen;
		return;
	}

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

	const WorldTransformComp* finalTargetTr =
		ecs.GetComponent<WorldTransformComp>(finalCand.entity);
	if (finalTargetTr != nullptr)
	{
		const double selfHomeDistSq =
			DistanceSqXZ(blackboard.homePosition, selfTr.position);
		const double targetHomeDistSq =
			DistanceSqXZ(blackboard.homePosition, finalTargetTr->position);
		const double leashDistSq = std::max(selfHomeDistSq, targetHomeDistSq);

		if (leashDistSq > hardLeashSq)
		{
			blackboard.leashGauge -= tuning.hardLeashDrainPerSec * sysCtx.dtSec;
		}
		else if (leashDistSq > softLeashSq)
		{
			blackboard.leashGauge -= tuning.leashDrainPerSec * sysCtx.dtSec;
		}
		else
		{
			blackboard.leashGauge += tuning.leashRecoverPerSec * sysCtx.dtSec;
		}

		blackboard.leashGauge = ClampDouble(
			blackboard.leashGauge,
			0.0,
			tuning.leashGaugeMax);

		if (blackboard.leashGauge <= 0.0)
		{
			EnterReturnHome(blackboard, perception);
			perception.timeSinceTargetLastSeen =
				blackboard.timeSinceCurrentTargetSeen;
			return;
		}
	}

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
