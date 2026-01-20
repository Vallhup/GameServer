#pragma once

#include "ECS.h"
#include "System.h"

class MapCollisionHandlingSystem : public System {
public:
	MapCollisionHandlingSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~MapCollisionHandlingSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return {  };
	}
};

