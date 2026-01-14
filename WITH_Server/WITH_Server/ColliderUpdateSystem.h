#pragma once

#include "ECS.h"
#include "System.h"

class ColliderUpdateSystem : public System {
public:
	ColliderUpdateSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ColliderUpdateSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Transform) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Collider) };
	}
};

