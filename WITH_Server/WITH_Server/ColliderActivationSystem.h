#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Action.h"

class ColliderActivationSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<ColliderActivationSystem>(),
		"ColliderActivationSystem",
		std::array{
			ReadImmediate(ComponentRes<ActionState>()),
			WriteImmediate(ComponentRes<AttackState>()),
			WriteImmediate(ComponentRes<CombatCollider>())
		}
	);

public:
	ColliderActivationSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~ColliderActivationSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
};

