#pragma once

#include "System.h"

struct AIState;
struct AIIntent;
struct AIThinkState;

class AIControlSystem : public System {
public:
	AIControlSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AIControlSystem() = default;

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
	void ApplyIdle(Entity entity);
	void ApplyMove(Entity entity, const AIIntent& intent);
	void ApplyAttack(Entity entity, AIState& aiState, AIThinkState& aiThinkState, AIIntent& intent);
};

