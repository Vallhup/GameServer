#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

struct LifecycleEvent;

class LifecycleReplicationSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<LifecycleReplicationSystem>(),
		"LifecycleReplicationSystem",
		std::array<AccessSpec, 0>{ }
	);

public:
	LifecycleReplicationSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~LifecycleReplicationSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void ProcessSpawn(const LifecycleEvent& event);
	void ProcessDespawn(const LifecycleEvent& event);
};

