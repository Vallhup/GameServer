#pragma once

#include "System.h"

class DirtyReplicationSystem : public System {
public:
	DirtyReplicationSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~DirtyReplicationSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return {  };
	}

private:
	void FlushEntity(Entity entity, const DirtyFlagsComp& dirty);

	void FlushTransform(Entity entity, NetId netId);
	void FlushAnimation(Entity entity, NetId netId);
	void FlushStat(Entity entity, NetId netId, uint32_t connId);
};

