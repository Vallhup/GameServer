#pragma once

#include "Event.h"
#include "System.h"

struct ActiveIndices {
	std::vector<uint16> offensiveHits;
	std::vector<uint16> defensiveHits;
	std::vector<uint16> hurts;
};

class CombatCollisionCheckSystem : public System {
public:
	CombatCollisionCheckSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~CombatCollisionCheckSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(CombatCollider), typeid(CombatCollisionEvent) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return {  };
	}

private:
	void BuildActiveIndices(const CombatCollider& collider, ActiveIndices* out);
	void CheckCollision(Entity attacker, const CombatCollider& aCol,
		const ActiveIndices& aActives, Entity victim,
		const CombatCollider& vCol, const ActiveIndices& vActives);

	template<typename EmitFunc>
	void CheckCollisionInternal(Entity attacker, const CombatCollider& aCol,
		const std::vector<uint16>& aOffHits, Entity victim, 
		const CombatCollider& vCol, const std::vector<uint16>& vTargets,
		EmitFunc&& emit);
};