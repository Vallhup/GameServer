#pragma once

#include "System.h"

class AIThinkSystem : public System {
public:
	AIThinkSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AIThinkSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Transform), typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(AIState), typeid(AIThinkState) };
	}

private:
	AttackType Think(Entity self, AIState* aiState);
	Entity FindTargetPlayer(Entity self, const AIState& aiState, const Transform& transform);
	Entity FindFarthestPlayer(Entity self, const Transform& selfTrans);
};

