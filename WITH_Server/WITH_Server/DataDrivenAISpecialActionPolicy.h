#pragma once

#include "AIBehaviorDef.h"
#include "IAISpecialActionPolicy.h"
#include "IAIState.h"

class DataDrivenAISpecialActionPolicy final : public IAISpecialActionPolicy {
public:
	virtual ~DataDrivenAISpecialActionPolicy() = default;

	virtual void TickRuntime(AIContext& ctx, const double dtSec) const override;
	virtual bool TryIssuePreFSMAction(AIContext& ctx) const override;

private:
	static const AIPhaseTransitionDef* ResolvePendingTransition(
		const AIContext& ctx,
		const AIPhaseRuntimeComp& phase) noexcept;

	static bool IsAbilityAvailableForSelf(
		const AIContext& ctx,
		AbilityId abilityId) noexcept;

	static void IssueTransitionAbility(
		AIContext& ctx,
		const AIPhaseTransitionDef& transition);

	static void ApplyTransitionRuntimeEffects(
		AIContext& ctx,
		const AIPhaseTransitionDef& transition) noexcept;

	static void ClearPendingTransition(AIPhaseRuntimeComp& phase) noexcept;
};
