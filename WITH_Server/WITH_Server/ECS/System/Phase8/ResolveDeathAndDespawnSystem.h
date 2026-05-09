#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ResolveDeathAndDespawnSystem final : public System {
	static const StaticSystemMetaStorage<6> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
