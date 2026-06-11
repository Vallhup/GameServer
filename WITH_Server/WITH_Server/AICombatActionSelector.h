#pragma once

#include "AICombatActionSelectionTypes.h"

class AICombatActionSelector final {
public:
	static AICombatActionChoice Select(
		const AICombatActionSelectionContext& context);

private:
	static bool IsEffectActionDue(
		const AICombatActionSelectionContext& context) noexcept;

	static std::vector<AICombatActionCandidate>
		CollectDueEffectCandidates(
			const AICombatActionSelectionContext& context,
			std::span<const AICombatActionCandidate> candidates);

	static std::vector<AICombatActionCandidate>
		BuildPreferredCandidateSet(
			const AICombatActionSelectionContext& context);

	static AICombatActionChoice PickWeightedCandidate(
		const AICombatActionSelectionContext& context,
		std::span<const AICombatActionCandidate> candidates) noexcept;
};
