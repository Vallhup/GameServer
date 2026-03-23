#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Stats.h"
#include "Bufs.h"
#include "Tags.h"

class StatRecalSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<StatRecalSystem>(),
		"StatRecalSystem",
		std::array{
			ReadImmediate(ComponentRes<BaseVital>()),
			ReadImmediate(ComponentRes<BaseAttribute>()),
			WriteImmediate(ComponentRes<FinalVital>()),
			WriteImmediate(ComponentRes<FinalAttribute>()),
			WriteImmediate(ComponentRes<BuffsComp>()),
			WriteImmediate(ComponentRes<Vital>())
		}
	);

public:
	StatRecalSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~StatRecalSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void ApplyBuff(Entity entity, const BuffInstance& inst, FinalVital& fVital, FinalAttribute& fAttr);
};

