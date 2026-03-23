#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Action.h"

class ActionTimeSystem final : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<ActionTimeSystem>(),
		"ActionTimeSystem",
		std::array{ WriteImmediate(ComponentRes<ActionState>()) }
	);

public:
	ActionTimeSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~ActionTimeSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
};

