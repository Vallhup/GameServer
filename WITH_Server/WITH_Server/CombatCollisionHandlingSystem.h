#pragma once

#include "Event.h"
#include "System.h"
#include "Bufs.h"
#include "Stats.h"
#include "Action.h"

class CombatCollisionHandlingSystem : public System {
public:
	CombatCollisionHandlingSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~CombatCollisionHandlingSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(FinalAttribute), typeid(ActionState), typeid(Transform) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(CombatCollisionEvent), typeid(ActionRequestEvent), typeid(AttackState), typeid(ParryBuf), typeid(Vital) };
	}

private:
	bool ConsumeHitOnce(const CombatCollisionEvent& event);

	void HandleClash(const CombatCollisionEvent& event);
	void HandleStrike(const CombatCollisionEvent& event);

	void HandleParry(Entity attacker, Entity victim, uint32 attackId);
	void HandleGuard(Entity attacker, Entity victim, uint32 attackId);
	void HandleHit(Entity attacker, Entity victim, uint32 attackId);
};

