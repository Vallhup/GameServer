#pragma once

#include "ECS.h"
#include "System.h"

class CombatCollisionHandlingSystem : public System {
public:
	CombatCollisionHandlingSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CombatCollisionHandlingSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(AttackData) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(CombatCollisionEvent), typeid(ParryBuf), typeid(Health) };
	}

private:
	bool ConsumeHitOnce(const CombatCollisionEvent& event);

	void HandleClash(const CombatCollisionEvent& event);
	void HandleStrike(const CombatCollisionEvent& event);

	void HandleParry(Entity attacker, Entity victim, uint32 attackId);
	void HandleGuard(Entity attacker, Entity victim, uint32 attackId);
	void HandleHit(Entity attacker, Entity victim, uint32 attackId);
};

