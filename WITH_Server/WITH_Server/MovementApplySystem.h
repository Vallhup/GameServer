#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Movement.h"

class MovementApplySystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<MovementApplySystem>(),
		"MovementApplySystem",
		std::array{
			WriteImmediate(ComponentRes<Transform>()),
			WriteImmediate(ComponentRes<ActionMoveDelta>()),
			WriteImmediate(ComponentRes<LocomotionMoveDelta>())
		}
	);

public:
	MovementApplySystem(WorldRuntime& rt) : System(rt) {}
	virtual ~MovementApplySystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void MovementApply(Entity entity, Transform* trans, 
		ActionMoveDelta* aDelta, LocomotionMoveDelta* lDelta, 
		const double dT);
};

