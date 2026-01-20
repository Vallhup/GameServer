#pragma once

#include "ECS.h"
#include "System.h"

class ColliderActivationSystem : public System {
public:
	ColliderActivationSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ColliderActivationSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(AttackState), typeid(CombatCollider) };
	}
};

