#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Action.h"
#include "Animation.h"
#include "Movement.h"

class AnimationSelectSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AnimationSelectSystem>(),
		"AnimationSelectSystem",
		std::array{
			ReadImmediate(ComponentRes<ActionState>()),
			ReadImmediate(ComponentRes<LocomotionState>()),
			WriteImmediate(ComponentRes<AnimationState>())
		}
	);

public:
	AnimationSelectSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AnimationSelectSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
};