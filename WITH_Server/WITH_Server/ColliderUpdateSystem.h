#pragma once

#include "System.h"

class ColliderUpdateSystem : public System {
public:
	ColliderUpdateSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ColliderUpdateSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Transform) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(CombatCollider) };
	}
};

