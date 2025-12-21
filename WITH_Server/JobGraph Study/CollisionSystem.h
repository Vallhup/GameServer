#pragma once

#include "ECS.h"
#include "System.h"

class CollisionSystem : public System {
public:
	CollisionSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CollisionSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Collider) };
	}
};

