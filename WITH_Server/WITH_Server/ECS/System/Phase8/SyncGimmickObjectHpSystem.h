#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class SyncGimmickObjectHpSystem final : public System {
	static const StaticSystemMetaStorage<8> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
