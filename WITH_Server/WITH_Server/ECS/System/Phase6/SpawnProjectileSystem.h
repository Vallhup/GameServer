#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class SpawnProjectileSystem final : public System {
	static const StaticSystemMetaStorage<4> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
