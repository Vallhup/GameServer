#include "pch.h"
#include "AIPerceptionSystem.h"

#include "../../../AIBehaviorDef.h"
#include "../../../GameDataCatalog.h"
#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"
#include "RepComponent.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<6> AIPerceptionSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<AIPerceptionSystem>(),
		"AIPerceptionSystem",
		std::array<AccessSpec, 6>
	{
		ReadImmediate(ComponentRes<AIControlledTag>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AITypeComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		WriteImmediate(ComponentRes<AIBlackboardComp>()),
		WriteImmediate(ComponentRes<AIPerceptionComp>()),
	});

void AIPerceptionSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, _, selfTr, aiType, blackboard, perception] :
		ctx.ecs.View<
		AIControlledTag, WorldTransformComp, AITypeComp,
		AIBlackboardComp, AIPerceptionComp>())
	{
		AIPerceptionTuningDef perceptionTuning{};
		AITargetingTuningDef  targetingTuning{};
		if (const AIBehaviorProfileDef* profile =
			GameDataCatalog::Current().AIBehaviors().Find(aiType.aiProfileId))
		{
			perceptionTuning = profile->perception;
			targetingTuning = profile->targeting;
		}

		BuildPerception(
			entity, selfTr, perceptionTuning, targetingTuning,
			blackboard, perception, ctx);
	}
}

void AIPerceptionSystem::EnterReturnHome(
	AIBlackboardComp& blackboard, 
	AIPerceptionComp& perception)
{
	blackboard.currentTarget = Entity::Null();
	blackboard.forceRetarget = false;
	blackboard.returningHome = true;
	blackboard.returnHomeLockoutAcc = 0.0;
	blackboard.leashGauge = 0.0;

	perception.Clear();
}

double AIPerceptionSystem::ComputeScore(
	const PerceptionCandidate& candidate,
	const AIPerceptionTuningDef& perception,
	const AITargetingTuningDef& targeting) const noexcept
{
	if (!candidate.inSightRange && !candidate.isCurrentTarget)
	{
		return std::numeric_limits<double>::lowest();
	}

	const double sightSq     = perception.sightRange * perception.sightRange;
	const double safeSightSq = std::max(0.001, sightSq);

	double distanceScore = 1.0 - (candidate.distSq / safeSightSq);
	distanceScore = std::clamp(distanceScore, 0.0, 1.0);

	double score{ distanceScore * 10.0 };
	if (candidate.isCurrentTarget)
		score += targeting.targetKeepBonus;

	if (candidate.isLastAttacker)
		score += targeting.lastAttackerBonus;

	if (candidate.inFront)
		score += targeting.frontBonus;

	return score;
}

AIPerceptionSystem::PerceptionCandidate AIPerceptionSystem::EvaluateCandidate(
	const WorldTransformComp& selfTr,
	const WorldTransformComp& otherTr,
	const AIPerceptionTuningDef& tuning,
	const AITargetingTuningDef& targeting,
	const AIBlackboardComp& blackboard,
	Entity other) const noexcept
{
	PerceptionCandidate out;
	out.entity          = other;
	out.isCurrentTarget = (blackboard.currentTarget == other);
	out.isLastAttacker  = (blackboard.lastAttacker == other);

	const double distSq = TransformHelper::DistanceSq(selfTr, otherTr);
	out.distSq = distSq;

	const double sightSq  = tuning.sightRange * tuning.sightRange;
	const double attackSq = tuning.attackRange * tuning.attackRange;

	out.inSightRange  = (distSq <= sightSq);
	out.inAttackRange = (distSq <= attackSq);

	const XMVECTOR selfForward = TransformHelper::Forward(selfTr);
	const XMVECTOR toTarget    = TransformHelper::Direction(selfTr, otherTr);

	out.forwardDot = TransformHelper::Dot(selfForward, toTarget);
	out.inFront	   = (out.forwardDot >= tuning.frontDotThreshold);
	out.visible	   = out.inSightRange;
	out.score      = ComputeScore(out, tuning, targeting);
	return out;
}

void AIPerceptionSystem::BuildPerception(
	Entity self,
	const WorldTransformComp& selfTr,
	const AIPerceptionTuningDef& tuning,
	const AITargetingTuningDef& targeting,
	AIBlackboardComp& blackboard,
	AIPerceptionComp& perception,
	SystemContext& sysCtx)
{
	ECSView& ecs = sysCtx.ecs;

	perception = {};

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

	for (const auto& [other, otherTr, _] :
		ecs.View<WorldTransformComp, SpawnTypeComp>())
	{
		if (other == self ||
			IsSameFaction(ecs, self, other))
		{
			continue;
		}

		const double homeDistSq       = TransformHelper::DistanceSq(blackboard.homePosition, otherTr.position);
		const bool   isCurrentTarget  = (blackboard.currentTarget == other);
		const bool   inAggroRange     = homeDistSq <= aggroSq;
		const bool   inHardLeashRange = homeDistSq <= hardLeashSq;
		const bool   canAcquire		  = !reaggroLocked && inAggroRange;
		const bool   canKeepCurrent   = isCurrentTarget;

		if (!canAcquire && !canKeepCurrent)
			continue;

		PerceptionCandidate cand = EvaluateCandidate(selfTr, otherTr, tuning, targeting, blackboard, other);
		cand.inSightRange = inAggroRange;
		cand.visible = isCurrentTarget ? inHardLeashRange : inAggroRange;
		cand.score = ComputeScore(cand, tuning, targeting);

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
				const double nextLeashGauge = blackboard.leashGauge + tuning.leashRecoverPerSec * sysCtx.dtSec;
				blackboard.leashGauge = std::clamp(nextLeashGauge, 0.0, tuning.leashGaugeMax);
			}
		}

		perception.Clear();
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
			best.score > currentCand.score + targeting.switchScoreMargin)
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
		
		if (const WorldTransformComp* targetTr =
			ecs.GetComponent<WorldTransformComp>(finalCand.entity))
		{
			blackboard.lastKnownTargetPosition    = targetTr->position;
			blackboard.hasLastKnownTargetPosition = true;
		}
	}
	else
	{
		blackboard.timeSinceCurrentTargetSeen += sysCtx.dtSec;
	}

	if (const WorldTransformComp* finalTargetTr = 
		ecs.GetComponent<WorldTransformComp>(finalCand.entity))
	{
		const double selfHomeDistSq   = TransformHelper::DistanceSq(blackboard.homePosition, selfTr.position);
		const double targetHomeDistSq = TransformHelper::DistanceSq(blackboard.homePosition, finalTargetTr->position);
		const double leashDistSq      = std::max(selfHomeDistSq, targetHomeDistSq);

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

		blackboard.leashGauge = std::clamp(blackboard.leashGauge, 0.0, tuning.leashGaugeMax);
		if (blackboard.leashGauge <= 0.0)
		{
			EnterReturnHome(blackboard, perception);
			return;
		}
	}

	perception.hasTarget		   = true;
	perception.selectedTarget	   = finalCand.entity;
	perception.distanceToTargetSq  = finalCand.distSq;
	perception.distanceToTarget	   = std::sqrt(finalCand.distSq);
	perception.targetForwardDot	   = finalCand.forwardDot;
	perception.targetVisible	   = finalCand.visible;
	perception.targetInSightRange  = finalCand.inSightRange;
	perception.targetInAttackRange = finalCand.inAttackRange;
	perception.targetInFront	   = finalCand.inFront;
}
