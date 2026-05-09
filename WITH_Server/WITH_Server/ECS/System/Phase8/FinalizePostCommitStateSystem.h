#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class FinalizePostCommitStateSystem final : public System {
	static const StaticSystemMetaStorage<1> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
