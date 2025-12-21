#pragma once

#include "System.h"
#include "Component.h"

class MovementSystem : public System {
public:
	MovementSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~MovementSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Velocity) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Transform) };
	}
};

