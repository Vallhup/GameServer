#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class RecoverStaminaSystem final : public System {
	static const StaticSystemMetaStorage<6, 0, 2> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
