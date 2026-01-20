#pragma once

#include "ECS.h"
#include "System.h"

class MapCollisionCheckSystem : public System {
public:
	MapCollisionCheckSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~MapCollisionCheckSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(MapCollider), typeid(OBB) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(MapCollisionEvent) };
	}

private:
	CapsuleView MakeCapsuleView(const MapCollider& collider);
};

