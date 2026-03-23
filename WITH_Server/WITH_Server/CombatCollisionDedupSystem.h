#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Event.h"

class CombatCollisionDedupSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<CombatCollisionDedupSystem>(),
		"CombatCollisionDedupSystem",
		std::array{
			WriteImmediate(EventRes<CombatCollisionEvent>())
		}
	);

public:
	CombatCollisionDedupSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~CombatCollisionDedupSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	std::span<CombatCollisionEvent> DedupCollisionEvent(std::span<CombatCollisionEvent> events);
};

