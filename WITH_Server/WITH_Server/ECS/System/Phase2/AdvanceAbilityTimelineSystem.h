#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

struct AbilityDef;
struct AbilityStateComp;
struct AbilityTimelineAdvanceComp;

class AdvanceAbilityTimelineSystem final : public System {
	static const StaticSystemMetaStorage<4> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static void ApplyElapsedToActiveAbility(
		AbilityStateComp& abilityState,
		const AbilityTimelineAdvanceComp& advance);

	static void CollectTimelineEvents(
		const AbilityDef& abilityDef,
		AbilityTimelineAdvanceComp& advance);
};
