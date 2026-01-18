#pragma once

#include "ECS.h"
#include "System.h"

class CollisionDedupSystem : public System {
public:
	CollisionDedupSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CollisionDedupSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(CollisionEvent) };
	}
};

