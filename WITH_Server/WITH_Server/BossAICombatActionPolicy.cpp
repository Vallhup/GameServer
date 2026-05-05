#include "pch.h"
#include "BossAICombatActionPolicy.h"

#include "AICombatActionPolicyUtil.h"
#include "AIBehaviorDef.h"
#include "GameplayContentCatalog.h"
#include "IAIState.h"
#include "System.h"

#include <algorithm>
#include <array>
#include <limits>
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

	AbilityId FindAbilityId(std::string_view key) noexcept
	{
		const GameplayContentCatalogSnapshot* catalog =
			GameplayContentCatalogSnapshot::TryCurrent();
		const AbilityDef* ability =
			catalog != nullptr ? catalog->FindAbilityByKey(key) : nullptr;
		return ability != nullptr ? ability->id : InvalidAbilityId;
	}

	size_t PatternCooldownIndex(AbilityId abilityId) noexcept
	{
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_6"))
			return 0;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_8"))
			return 1;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_7"))
			return 2;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_3"))
			return 3;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_4"))
			return 4;
		return std::numeric_limits<size_t>::max();
	}

	float PatternCooldownDuration(AbilityId abilityId) noexcept
	{
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_6"))
			return 1.0f;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_8"))
			return 1.5f;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_7"))
			return 2.5f;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_3"))
			return 4.0f;
		if (abilityId == FindAbilityId("Ability.BigDemonWarrior_Melee_4"))
			return 6.0f;
		return 0.0f;
	}

	bool IsPatternCooldownReady(
		const BossPatternRuntimeComp* runtime,
		AbilityId abilityId) noexcept
	{
		if (runtime == nullptr)
		{
			return true;
		}

		const size_t index = PatternCooldownIndex(abilityId);
		return
			index >= runtime->patternCooldownSec.size() ||
			runtime->patternCooldownSec[index] <= 0.0f;
	}

	void ReservePatternCooldown(
		BossPatternRuntimeComp* runtime,
		AbilityId abilityId) noexcept
	{
		if (runtime == nullptr)
		{
			return;
		}

		const size_t index = PatternCooldownIndex(abilityId);
		if (index == std::numeric_limits<size_t>::max())
		{
			return;
		}
		if (index >= runtime->patternCooldownSec.size())
		{
			runtime->patternCooldownSec.resize(index + 1, 0.0f);
		}

		runtime->patternCooldownSec[index] =
			PatternCooldownDuration(abilityId);
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
		AbilityId abilityId,
		uint16_t weight,
		AbilityId lastUsed,
		const BossPatternRuntimeComp* runtime,
		bool forbidImmediateRepeat = false) noexcept
	{
		if (count >= candidates.size())
			return;

		if (forbidImmediateRepeat && abilityId == lastUsed)
			return;

		if (!IsPatternCooldownReady(runtime, abilityId))
			return;

		candidates[count++] = WeightedActionEntry{
			.abilityId = abilityId,
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

		const AbilityId lastUsed = ctx.blackboard
			? ctx.blackboard->lastUsedAbilityId
			: InvalidAbilityId;

		if (phase <= 1)
		{
			switch (bucket) {
			case BossDistanceBucket::VeryClose:
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_6"),
					100,
					lastUsed,
					runtime);
				break;
			case BossDistanceBucket::Close:
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_8"),
					70,
					lastUsed,
					runtime);
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_7"),
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
					FindAbilityId("Ability.BigDemonWarrior_Melee_6"),
					55,
					lastUsed,
					runtime);
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_7"),
					45,
					lastUsed,
					runtime,
					true);
				break;
			case BossDistanceBucket::Close:
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_8"),
					45,
					lastUsed,
					runtime);
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_7"),
					30,
					lastUsed,
					runtime,
					true);
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_3"),
					25,
					lastUsed,
					runtime,
					true);
				break;
			case BossDistanceBucket::Mid:
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_3"),
					45,
					lastUsed,
					runtime,
					true);
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_4"),
					55,
					lastUsed,
					runtime,
					true);
				break;
			case BossDistanceBucket::Far:
				AddCandidate(
					candidates,
					count,
					FindAbilityId("Ability.BigDemonWarrior_Melee_4"),
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

CombatActionSelection BossAICombatActionPolicy::SelectAction(
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
	const AbilityId selectedAbilityId =
		AICombatActionPolicyUtil::PickWeightedAbility(
			candidateSpan,
			ctx.blackboard->lastUsedAbilityId,
			roll,
			35);

	if (selectedAbilityId == InvalidAbilityId)
		return result;

	result.shouldAttack = true;
	result.selectedAbilityId = selectedAbilityId;
	ReservePatternCooldown(runtime, selectedAbilityId);
	AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	return result;
}
