#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Event.h"

struct ActiveIndices {
	std::vector<uint16> offensiveHits;
	std::vector<uint16> defensiveHits;
	std::vector<uint16> hurts;
};

class CombatCollisionCheckSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<CombatCollisionCheckSystem>(),
		"CombatCollisionCheckSystem",
		std::array{  
			ReadImmediate(ComponentRes<CombatCollider>()),
			ReadImmediate(EventRes<CombatCollisionEvent>())
		}
	);

public:
	CombatCollisionCheckSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~CombatCollisionCheckSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

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