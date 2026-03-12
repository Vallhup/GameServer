#pragma once

#include "System.h"

struct LifecycleEvent;

class LifecycleReplicationSystem : public System {
public:
	LifecycleReplicationSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~LifecycleReplicationSystem() = default;

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
	void ProcessSpawn(const LifecycleEvent& event);
	void ProcessDespawn(const LifecycleEvent& event);
};

