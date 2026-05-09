#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class ComputeLocomotionMoveDeltaSystem final : public System {
	static const StaticSystemMetaStorage<5> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
