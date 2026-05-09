#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ResolveCharacterOverlapSystem final : public System {
	static const StaticSystemMetaStorage<7> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
