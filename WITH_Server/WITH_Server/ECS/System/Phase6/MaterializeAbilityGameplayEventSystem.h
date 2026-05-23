#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class MaterializeAbilityGameplayEventSystem final : public System {
	static const StaticSystemMetaStorage<9> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
