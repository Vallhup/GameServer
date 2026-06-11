#include "pch.h"
#include "AITargetAttackTracker.h"

#include "AIBehaviorDef.h"
#include "IAIState.h"

void AITargetAttackTracker::RecordCommittedAttack(
	AIContext& ctx,
	Entity target) noexcept
{
	if (ctx.blackboard == nullptr ||
		ctx.behaviorProfile == nullptr ||
		target.IsNull())
	{
		return;
	}

	const int threshold =
		ctx.behaviorProfile->targeting.attacksBeforeForcedRetarget;
	if (threshold <= 0)
		return;

	AIBlackboardComp& blackboard = *ctx.blackboard;
	if (blackboard.attackCountTarget != target)
		ResetForTarget(blackboard, target);

	blackboard.committedAttackCount =
		static_cast<uint16_t>(std::min<uint32_t>(
			static_cast<uint32_t>(
				blackboard.committedAttackCount) + 1u,
			std::numeric_limits<uint16_t>::max()));

	if (blackboard.committedAttackCount <
		static_cast<uint32_t>(threshold))
	{
		return;
	}

	blackboard.committedAttackCount = 0;
	blackboard.alternateTargetRetargetRequested = true;
}

void AITargetAttackTracker::ResetForTarget(
	AIBlackboardComp& blackboard,
	Entity target) noexcept
{
	blackboard.attackCountTarget = target;
	blackboard.committedAttackCount = 0;
	blackboard.alternateTargetRetargetRequested = false;
}
