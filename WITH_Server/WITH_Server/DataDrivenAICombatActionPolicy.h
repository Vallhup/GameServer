#pragma once

#include "AIBehaviorDef.h"
#include "IAICombatActionPolicy.h"
#include "IAIState.h"

class DataDrivenAICombatActionPolicy final : public IAICombatActionPolicy {
public:
	virtual ~DataDrivenAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(const AIContext& ctx) const override;

private:
	struct CombatActionCandidate
	{
		size_t   actionIndex{ 0 };
		uint32_t weight{ 0 };
	};

	struct CombatActionSelectionContext
	{
		const AIContext&				aiCtx;
		AIActionRuntimeComp*			runtime{ nullptr };
		std::span<const AIActionDef>	actions;

		const AIMovementProfileDef*		movement{ nullptr };
		AIActionDistanceBucket			currentBucket{ AIActionDistanceBucket::Any };
		double							distanceToTarget{ 0.0 };

		uint8_t		phase{ 1 };
		float		selfHpRatio{ 1.0f };
		float		targetHpRatio{ 1.0f };
		AbilityId	lastUsedAbilityId{ InvalidAbilityId };
		uint32_t	actionSequence{ 0 };
		uint16_t	basicActionCountSinceEffect{ 0 };
	};

	static bool CanSelectCombatAction(const AIContext& ctx) noexcept;

	static CombatActionSelectionContext BuildSelectionContext(
		const AIContext& ctx) noexcept;

	static float ResolveHpRatio(const CombatStatStateComp* stats) noexcept;

	static std::vector<CombatActionCandidate> CollectCandidates(
		const CombatActionSelectionContext& selection);

	static bool IsEffectActionDue(
		const CombatActionSelectionContext& selection) noexcept;

	static void KeepDueEffectCandidatesOnly(
		const CombatActionSelectionContext& selection,
		std::vector<CombatActionCandidate>& candidates);

	static const AIActionDef* PickCombatAction(
		const CombatActionSelectionContext& selection,
		std::span<const CombatActionCandidate> candidates,
		size_t& outActionIndex) noexcept;

	static void ApplyRuntimeSelection(
		const CombatActionSelectionContext& selection,
		const AIActionDef& action,
		size_t actionIndex);
};
