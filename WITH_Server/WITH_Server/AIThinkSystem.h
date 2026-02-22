#pragma once

#include "Event.h"
#include "System.h"

struct AIState;

class AIThinkSystem : public System {
public:
	AIThinkSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AIThinkSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { };
	}

private:
	AttackType Think(Entity self, AIState* aiState);
	Entity FindTargetPlayer(Entity self, const AIState& aiState, const Transform& transform);
	Entity FindFarthestPlayer(Entity self, const Transform& selfTrans);
};

