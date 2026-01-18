#pragma once

#include "ECS.h"
#include "System.h"

class CollisionHandlingSystem : public System {
public:
	CollisionHandlingSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CollisionHandlingSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(AttackData) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(CollisionEvent), typeid(ParryBuf), typeid(Health) };
	}

private:
	void HandleClash(const CollisionEvent& event);
	void HandleStrike(const CollisionEvent& event);

	void HandleParry(Entity attacker, Entity victim, uint32 attackId);
	void HandleGuard(Entity attacker, Entity victim, uint32 attackId);
	void HandleHit(Entity attacker, Entity victim, uint32 attackId);
};

