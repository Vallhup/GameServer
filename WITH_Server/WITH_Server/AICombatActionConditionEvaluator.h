#pragma once

#include "AICombatActionSelectionTypes.h"

class AICombatActionConditionEvaluator final {
public:
	static std::vector<AICombatActionCandidate> CollectEligibleCandidates(
		const AICombatActionSelectionContext& context,
		AIImmediateRepeatMode repeatMode);

	static bool MatchesPhase(
		const AICombatActionSelectionContext& context,
		const AIActionConditionDef& condition) noexcept;

private:
	static bool MatchesSpatialConditions(
		const AICombatActionSelectionContext& context,
		const AIActionConditionDef& condition) noexcept;

	static bool MatchesHealthConditions(
		const AICombatActionSelectionContext& context,
		const AIActionConditionDef& condition) noexcept;

	static bool MatchesPerceptionConditions(
		const AICombatActionSelectionContext& context,
		const AIActionConditionDef& condition) noexcept;

	static bool MatchesAbilityConditions(
		const AICombatActionSelectionContext& context,
		const AIActionDef& action) noexcept;

	static bool MatchesCooldownConditions(
		const AICombatActionSelectionContext& context,
		const AIActionDef& action,
		size_t actionIndex) noexcept;

	static bool MatchesActionSequenceConditions(
		const AICombatActionSelectionContext& context,
		const AIActionDef& action) noexcept;

	static uint32_t ResolveWeight(
		const AICombatActionSelectionContext& context,
		const AIActionDef& action,
		AIImmediateRepeatMode repeatMode) noexcept;
};
