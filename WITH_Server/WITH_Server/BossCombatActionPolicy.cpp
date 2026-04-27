#include "pch.h"
#include "BossCombatActionPolicy.h"

#include "AICombatActionPolicyUtil.h"
#include "IAIState.h"
#include "System.h"

#include <algorithm>
#include <array>
#include <span>

namespace
{
	enum class BossDistanceBucket : uint8_t
	{
		VeryClose,
		Close,
		Mid,
		Far
	};

	constexpr double kVeryCloseDistance = 2.2;
	constexpr double kCloseDistance = 5.8;
	constexpr double kMidDistance = 7.5;

	size_t PatternCooldownIndex(ActionId actionId) noexcept
	{
		switch (actionId) {
		case ActionId::BigDemonWarrior_Melee_6:
			return 0;
		case ActionId::BigDemonWarrior_Melee_8:
			return 1;
		case ActionId::BigDemonWarrior_Melee_7:
			return 2;
		case ActionId::BigDemonWarrior_Melee_3:
			return 3;
		case ActionId::BigDemonWarrior_Melee_4:
			return 4;
		default:
			return BossPatternRuntimeComp::kPatternCooldownSlotCount;
		}
	}

	float PatternCooldownDuration(ActionId actionId) noexcept
	{
		switch (actionId) {
		case ActionId::BigDemonWarrior_Melee_6:
			return 1.0f;
		case ActionId::BigDemonWarrior_Melee_8:
			return 1.5f;
		case ActionId::BigDemonWarrior_Melee_7:
			return 2.5f;
		case ActionId::BigDemonWarrior_Melee_3:
			return 4.0f;
		case ActionId::BigDemonWarrior_Melee_4:
			return 6.0f;
		default:
			return 0.0f;
		}
	}

	bool IsPatternCooldownReady(
		const BossPatternRuntimeComp* runtime,
		ActionId actionId) noexcept
	{
		if (runtime == nullptr)
		{
			return true;
		}

		const size_t index = PatternCooldownIndex(actionId);
		return
			index >= BossPatternRuntimeComp::kPatternCooldownSlotCount ||
			runtime->patternCooldownSec[index] <= 0.0f;
	}

	void ReservePatternCooldown(
		BossPatternRuntimeComp* runtime,
		ActionId actionId) noexcept
	{
		if (runtime == nullptr)
		{
			return;
		}

		const size_t index = PatternCooldownIndex(actionId);
		if (index >= BossPatternRuntimeComp::kPatternCooldownSlotCount)
		{
			return;
		}

		runtime->patternCooldownSec[index] =
			PatternCooldownDuration(actionId);
	}

	BossDistanceBucket ResolveDistanceBucket(double distance) noexcept
	{
		if (distance <= kVeryCloseDistance)
			return BossDistanceBucket::VeryClose;
		if (distance <= kCloseDistance)
			return BossDistanceBucket::Close;
		if (distance <= kMidDistance)
			return BossDistanceBucket::Mid;
		return BossDistanceBucket::Far;
	}

	uint8_t ResolvePhase(const AIContext& ctx) noexcept
	{
		if (ctx.sysCtx == nullptr)
			return 1;

		const BossPhaseStateComp* phase =
			ctx.sysCtx->ecs.GetComponent<BossPhaseStateComp>(ctx.self);
		return phase ? std::max<uint8_t>(1, phase->currentPhase) : 1;
	}

	void AddCandidate(
		std::array<WeightedActionEntry, 5>& candidates,
		size_t& count,
		ActionId actionId,
		uint16_t weight,
		ActionId lastUsed,
		const BossPatternRuntimeComp* runtime,
		bool forbidImmediateRepeat = false) noexcept
	{
		if (count >= candidates.size())
			return;

		if (forbidImmediateRepeat && actionId == lastUsed)
			return;

		if (!IsPatternCooldownReady(runtime, actionId))
			return;

		candidates[count++] = WeightedActionEntry{
			.actionId = actionId,
			.weight = weight
		};
	}

	std::span<const WeightedActionEntry> BuildCandidates(
		const AIContext& ctx,
		uint8_t phase,
		BossDistanceBucket bucket,
		const BossPatternRuntimeComp* runtime,
		std::array<WeightedActionEntry, 5>& candidates,
		size_t& count) noexcept
	{
		count = 0;

		const ActionId lastUsed = ctx.blackboard
			? ctx.blackboard->lastUsedActionId
			: ActionId::None;

		if (phase <= 1)
		{
			switch (bucket) {
			case BossDistanceBucket::VeryClose:
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_6,
					100,
					lastUsed,
					runtime);
				break;
			case BossDistanceBucket::Close:
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_8,
					70,
					lastUsed,
					runtime);
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_7,
					30,
					lastUsed,
					runtime,
					true);
				break;
			default:
				break;
			}
		}
		else
		{
			switch (bucket) {
			case BossDistanceBucket::VeryClose:
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_6,
					55,
					lastUsed,
					runtime);
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_7,
					45,
					lastUsed,
					runtime,
					true);
				break;
			case BossDistanceBucket::Close:
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_8,
					45,
					lastUsed,
					runtime);
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_7,
					30,
					lastUsed,
					runtime,
					true);
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_3,
					25,
					lastUsed,
					runtime,
					true);
				break;
			case BossDistanceBucket::Mid:
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_3,
					45,
					lastUsed,
					runtime,
					true);
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_4,
					55,
					lastUsed,
					runtime,
					true);
				break;
			case BossDistanceBucket::Far:
				AddCandidate(
					candidates,
					count,
					ActionId::BigDemonWarrior_Melee_4,
					100,
					lastUsed,
					runtime,
					true);
				break;
			}
		}

		return std::span<const WeightedActionEntry>(
			candidates.data(),
			count);
	}
}

CombatActionSelection BossCombatActionPolicy::SelectAction(
	const AIContext& ctx) const
{
	CombatActionSelection result{};

	if (ctx.decision == nullptr ||
		ctx.decisionTuning == nullptr ||
		ctx.blackboard == nullptr ||
		ctx.perception == nullptr ||
		ctx.decision->attackCooldownAcc < ctx.decisionTuning->attackCooldown)
	{
		return result;
	}

	const uint8_t phase = ResolvePhase(ctx);
	const BossDistanceBucket bucket =
		ResolveDistanceBucket(ctx.perception->distanceToTarget);
	BossPatternRuntimeComp* runtime = ctx.sysCtx
		? ctx.sysCtx->ecs.GetMutableComponent<BossPatternRuntimeComp>(ctx.self)
		: nullptr;

	if (runtime != nullptr && runtime->phaseTransitionLockSec > 0.0f)
	{
		return result;
	}

	std::array<WeightedActionEntry, 5> candidates{};
	size_t candidateCount = 0;
	const std::span<const WeightedActionEntry> candidateSpan =
		BuildCandidates(ctx, phase, bucket, runtime, candidates, candidateCount);

	if (candidateSpan.empty())
		return result;

	const int roll = AICombatActionPolicyUtil::PseudoRand(
		ctx.self.id,
		ctx.blackboard->combatActionSequence,
		10000);
	const ActionId selectedActionId =
		AICombatActionPolicyUtil::PickWeightedAction(
			candidateSpan,
			ctx.blackboard->lastUsedActionId,
			roll,
			35);

	if (selectedActionId == ActionId::None)
		return result;

	result.shouldAttack = true;
	result.selectedActionId = selectedActionId;
	ReservePatternCooldown(runtime, selectedActionId);
	AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	return result;
}
