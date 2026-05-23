#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ResolveAreaHitSystem final : public System {
	static const StaticSystemMetaStorage<13> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
