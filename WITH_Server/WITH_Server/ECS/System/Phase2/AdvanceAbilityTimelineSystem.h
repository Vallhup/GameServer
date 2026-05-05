#pragma once

#include "System.h"
#include "../../GameplayRuntimeComponents.h"

class AdvanceAbilityTimelineSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static const SystemMeta kMeta;

	static void ApplyElapsedToActiveAbility(
		AbilityStateComp& abilityState,
		const AbilityTimelineAdvanceComp& advance);

	static void CollectTimelineEvents(
		const AbilityDef& abilityDef,
		AbilityTimelineAdvanceComp& advance);
};
