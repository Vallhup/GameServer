#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

class ColliderUpdateSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<ColliderUpdateSystem>(),
		"ColliderUpdateSystem",
		std::array{
			ReadImmediate(ComponentRes<Transform>()),
			WriteImmediate(ComponentRes<CombatCollider>())
		}
	);

public:
	ColliderUpdateSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~ColliderUpdateSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
};

