#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

class DirtyReplicationSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<DirtyReplicationSystem>(),
		"DirtyReplicationSystem",
		std::array<AccessSpec, 0>{ }
	);

public:
	DirtyReplicationSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~DirtyReplicationSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void FlushEntity(Entity entity, const DirtyFlagsComp& dirty);

	void FlushTransform(Entity entity, NetId netId);
	void FlushAnimation(Entity entity, NetId netId);
	void FlushStat(Entity entity, NetId netId, uint32_t connId);
};

