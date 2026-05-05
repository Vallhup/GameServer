#include "pch.h"
#include "AIIdleState.h"

#include "AIBehaviorDef.h"

namespace
{
	static int PseudoRand(uint64_t entityId, uint32_t sequence, int range)
	{
		if (range <= 0)
			return 0;

		uint64_t x =
			entityId ^
			(static_cast<uint64_t>(sequence) + 0x9e3779b97f4a7c15ull);
		x ^= (x >> 30);
		x *= 0xbf58476d1ce4e5b9ull;
		x ^= (x >> 27);
		x *= 0x94d049bb133111ebull;
		x ^= (x >> 31);
		return static_cast<int>(x % static_cast<uint64_t>(range));
	}

	static AbilityId SelectWeightedAbility(
		std::span<const WeightedActionEntry> actions,
		int roll)
	{
		uint32_t totalWeight = 0;
		for (const WeightedActionEntry& entry : actions)
			totalWeight += entry.weight;

		if (totalWeight == 0)
			return InvalidAbilityId;

		uint32_t cursor = static_cast<uint32_t>(roll) % totalWeight;
		for (const WeightedActionEntry& entry : actions)
		{
			if (entry.weight == 0)
				continue;

			if (cursor < entry.weight)
				return entry.abilityId;

			cursor -= entry.weight;
		}

		return InvalidAbilityId;
	}

	static void TryIssueIdleAbility(AIContext& ctx)
	{
		if (ctx.behaviorProfile == nullptr ||
			ctx.blackboard == nullptr ||
			ctx.command == nullptr ||
			ctx.abilityState == nullptr ||
			ctx.perceptionTuning == nullptr ||
			ctx.behaviorProfile->idleActions.empty() ||
			!ctx.abilityState->CanIssueAbility())
		{
			return;
		}

		if (ctx.blackboard->idleActionCooldownAcc <
			ctx.perceptionTuning->idleActionCooldownSec)
		{
			return;
		}

		const uint32_t sequence = ctx.blackboard->idleActionSequence++;
		const int chanceRoll = PseudoRand(ctx.self.id, sequence, 100);
		ctx.blackboard->idleActionCooldownAcc = 0.0;

		if (chanceRoll >= ctx.perceptionTuning->idleActionChancePercent)
			return;

		const int abilityRoll =
			PseudoRand(ctx.self.id, sequence + 0x85ebca6bu, 10000);
		const AbilityId abilityId =
			SelectWeightedAbility(ctx.behaviorProfile->idleActions, abilityRoll);
		if (abilityId == InvalidAbilityId)
			return;

		ctx.command->hasAbility = true;
		ctx.command->abilityId = abilityId;
		ctx.command->abilityDirX = 0.0f;
		ctx.command->abilityDirZ = 0.0f;
		ctx.command->sequence++;
	}
}

void AIIdleState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
}

void AIIdleState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	(void)decisionDT;

	if (ctx.blackboard->returningHome)
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
		return;
	}

	TryIssueIdleAbility(ctx);
}

void AIIdleState::FrameUpdate(AIContext& ctx, const double dT) const
{
}
