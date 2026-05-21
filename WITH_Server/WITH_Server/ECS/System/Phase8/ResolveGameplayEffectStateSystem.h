#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ResolveGameplayEffectStateSystem final : public System {
	static const StaticSystemMetaStorage<12, 0, 3> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
