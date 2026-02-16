#pragma once

#include "System.h"

class ColliderActivationSystem : public System {
public:
	ColliderActivationSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ColliderActivationSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(AttackState), typeid(CombatCollider) };
	}
};

