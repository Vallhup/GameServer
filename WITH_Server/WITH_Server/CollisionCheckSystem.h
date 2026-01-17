#pragma once

#include "ECS.h"
#include "System.h"

struct CapsuleView {
	XMFLOAT3 p0;
	XMFLOAT3 p1;
	float radius;
};

struct ActiveIndices {
	std::vector<uint16> offensiveHits;
	std::vector<uint16> defensiveHits;
	std::vector<uint16> hurts;
};

class CollisionCheckSystem : public System {
public:
	CollisionCheckSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~CollisionCheckSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return {  };
	}

private:
	inline CapsuleView MakeCapsuleView(const Collider& collider, size_t i);
	void BuildActiveIndices(const Collider& collider, ActiveIndices* out);
	void CheckCollision(Entity attacker, const Collider& aCol,
		const ActiveIndices& aActives, Entity victim,
		const Collider& vCol, const ActiveIndices& vActives);

	template<typename EmitFunc>
	void CheckCollisionInternal(Entity attacker, const Collider& aCol, 
		const std::vector<uint16>& aOffHits, Entity victim, 
		const Collider& vCol, const std::vector<uint16>& vTargets,
		EmitFunc&& emit);
};

template<typename EmitFunc>
inline void CollisionCheckSystem::CheckCollisionInternal(Entity attacker,
	const Collider& aCol, const std::vector<uint16>& aOffHits, Entity victim, 
	const Collider& vCol, const std::vector<uint16>& vTargets, EmitFunc&& emit)
{
	for (uint16 aOffHit : aOffHits)
	{
		const uint32 atkId = aCol.attackIds[aOffHit];
		const CapsuleView hitCap = MakeCapsuleView(aCol, aOffHit);

		for (uint16 vTarget : vTargets)
		{
			const CapsuleView hurtCap = MakeCapsuleView(vCol, vTarget);

			if (Collision::CheckCapsuleVsCapsule(hitCap, hurtCap))
				emit(attacker, victim, atkId, aOffHit, vTarget);
		}
	}
}
