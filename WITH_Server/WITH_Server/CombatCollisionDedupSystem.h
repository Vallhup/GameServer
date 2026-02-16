#pragma once

#include "Event.h"
#include "System.h"

class CombatCollisionDedupSystem : public System {
public:
	CombatCollisionDedupSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~CombatCollisionDedupSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(CombatCollisionEvent) };
	}

private:
	std::span<CombatCollisionEvent> DedupCollisionEvent(std::span<CombatCollisionEvent> events);
};

