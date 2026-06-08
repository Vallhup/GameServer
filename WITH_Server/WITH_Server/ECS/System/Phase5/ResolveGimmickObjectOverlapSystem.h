#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ResolveGimmickObjectOverlapSystem final : public System {
	static const StaticSystemMetaStorage<10> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
