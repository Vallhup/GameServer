#pragma once

#include "System.h"

class AIThinkSystem : public System {
public:
	AIThinkSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AIThinkSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Transform), typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(AIState), typeid(AIThinkState) };
	}

private:
	AttackType Think(Entity self, AIState* aiState);
	Entity FindTargetPlayer(Entity self, const AIState& aiState, const Transform& transform);
	Entity FindFarthestPlayer(Entity self, const Transform& selfTrans);
};

