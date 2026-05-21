#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

struct CombatStatStateComp;

class CommitCombatResultSystem final : public System {
	static const StaticSystemMetaStorage<16> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static bool DidStatsChange(
		const CombatStatStateComp& previousStats,
		const CombatStatStateComp& currentStats) noexcept;
};
