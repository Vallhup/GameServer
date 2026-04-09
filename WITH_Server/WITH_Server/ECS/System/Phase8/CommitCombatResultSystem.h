#pragma once

#include "System.h"
#include "../../GameplayRuntimeComponents.h"

class CommitCombatResultSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static bool DidStatsChange(
		const CombatStatStateComp& previousStats,
		const CombatStatStateComp& currentStats) noexcept;

	static const SystemMeta kMeta;
};
