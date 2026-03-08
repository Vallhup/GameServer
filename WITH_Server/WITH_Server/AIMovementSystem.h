#pragma once

#include "System.h"
#include "AI.h"
#include "Movement.h"
#include "Action.h"

class AIMovementSystem : public System {
public:
	AIMovementSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AIMovementSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(ActionState), typeid(Transform), typeid(AIState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(Velocity) };
	}

private:


};

