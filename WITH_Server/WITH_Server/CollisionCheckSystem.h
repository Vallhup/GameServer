#pragma once

#include "ECS.h"
#include "System.h"

struct CapsuleView;

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
		return { typeid(Collider) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(CollisionEvent) };
	}

private:
	CapsuleView MakeCapsuleView(const Collider& collider, size_t i);
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