#include "pch.h"
#include "AIReturnHomeState.h"

#include "AIBehaviorDef.h"
#include "IAIMovementPolicy.h"
#include "RepComponent.h"
#include "TransformHelper.h"

#include <algorithm>
#include <cmath>

bool AIReturnHomeState::HasArrivedHome(const AIContext& ctx) noexcept
{
	if (ctx.selfTr == nullptr ||
		ctx.blackboard == nullptr ||
		ctx.perceptionTuning == nullptr ||
		!ctx.blackboard->hasHomePosition)
	{
		return true;
	}

	const double distanceSq = TransformHelper::DistanceSq(ctx.selfTr->position, ctx.blackboard->homePosition);
	const double arriveRange = ctx.perceptionTuning->returnHomeArriveRange;

	return distanceSq <= arriveRange * arriveRange;
}

void AIReturnHomeState::RecoverHp(AIContext& ctx, const double dT)
{
	if (ctx.sysCtx == nullptr ||
		ctx.stats == nullptr ||
		ctx.blackboard == nullptr ||
		ctx.perceptionTuning == nullptr ||
		ctx.stats->maxHp <= 0 ||
		ctx.stats->currentHp <= 0 ||
		ctx.stats->currentHp >= ctx.stats->maxHp)
	{
		return;
	}

	ctx.blackboard->returnHpRegenAcc +=
		static_cast<double>(ctx.stats->maxHp) *
		ctx.perceptionTuning->returnHpRegenPerSecRatio * dT;

	const int32_t recoverAmount = static_cast<int32_t>(std::floor(ctx.blackboard->returnHpRegenAcc));
	if (recoverAmount <= 0)
		return;

	ctx.blackboard->returnHpRegenAcc -= static_cast<double>(recoverAmount);

	const int32_t oldHp = ctx.stats->currentHp;
	ctx.stats->currentHp = std::clamp(
		ctx.stats->currentHp + recoverAmount, 0, ctx.stats->maxHp);

	if (ctx.stats->currentHp != oldHp)
	{
		if (DirtyFlagsComp* dirty =
			ctx.sysCtx->ecs.GetMutableComponent<DirtyFlagsComp>(ctx.self))
		{
			dirty->MarkDirty(WorldDirtyType::Stat);
		}
	}
}

void AIReturnHomeState::ClearReturnTarget(AIContext& ctx)
{
	if (ctx.blackboard)
	{
		ctx.blackboard->currentTarget			   = Entity::Null();
		ctx.blackboard->hasLastKnownTargetPosition = false;
	}

	if (ctx.intent)
	{
		ctx.intent->target	   = Entity::Null();
		ctx.intent->hasLook	   = false;
		ctx.intent->hasAbility = false;
		ctx.intent->abilityId  = InvalidAbilityId;
	}
}

void AIReturnHomeState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();

	if (ctx.blackboard)
	{
		ctx.blackboard->returningHome	 = true;
		ctx.blackboard->returnHpRegenAcc = 0.0;
	}

	ClearReturnTarget(ctx);
}

void AIReturnHomeState::DecisionUpdate(
	AIContext& ctx,
	const double decisionDT) const
{
	if (!HasArrivedHome(ctx))
		return;

	if (ctx.blackboard)
	{
		ctx.blackboard->returningHome        = false;
		ctx.blackboard->returnHomeLockoutAcc = 0.0;

		if (ctx.perceptionTuning)
		{
			ctx.blackboard->leashGauge = ctx.perceptionTuning->leashGaugeMax;
		}
	}

	if (ctx.movementRuntime)
	{
		ctx.movementRuntime->hasPathCorner    = false;
		ctx.movementRuntime->pathRecomputeAcc = 0.0;
	}

	ClearReturnTarget(ctx);
	ctx.decision->RequestTransition(AIStateType::Idle);
}

void AIReturnHomeState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ClearReturnTarget(ctx);
	RecoverHp(ctx, dT);

	if (ctx.movementPolicy)
		ctx.movementPolicy->BuildReturnHomeIntent(ctx);
}
