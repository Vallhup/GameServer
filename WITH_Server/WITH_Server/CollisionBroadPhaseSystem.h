#pragma once

#include "ECS.h"
#include "System.h"

class CollisionBroadPhaseSystem : public System {
public:
	CollisionBroadPhaseSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CollisionBroadPhaseSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(OBB) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(MapCollisionPair) };
	}
};

