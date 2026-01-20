#pragma once

#include "ECS.h"
#include "System.h"

struct ActiveIndices {
	std::vector<uint16> offensiveHits;
	std::vector<uint16> defensiveHits;
	std::vector<uint16> hurts;
};

class CombatCollisionCheckSystem : public System {
public:
	CombatCollisionCheckSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CombatCollisionCheckSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(CombatCollider) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(CombatCollisionEvent) };
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