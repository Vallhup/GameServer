#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Event.h"
#include "BuffData.h"
#include "Bufs.h"

class BuffApplySystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<BuffApplySystem>(),
		"BuffApplySystem",
		std::array{
			WriteImmediate(ComponentRes<BuffsComp>()),
			WriteImmediate(EventRes<DeathEvent>())
		}
	);

public:
	BuffApplySystem(WorldRuntime& rt) : System(rt) {}
	virtual ~BuffApplySystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void FindTargetEntities(const ECS& ecs, std::vector<Entity>& out);
	void GiveBuff(Entity target, BuffType type);
};

