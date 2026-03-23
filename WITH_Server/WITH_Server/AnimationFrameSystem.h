#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Action.h"
#include "Animation.h"

class AnimationFrameSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AnimationFrameSystem>(),
		"AnimationFrameSystem",
		std::array{
			ReadImmediate(ComponentRes<ActionState>()),
			ReadImmediate(ComponentRes<AnimationState>()),
			ReadImmediate(ComponentRes<LocomotionAnimPhase>()),
			WriteImmediate(ComponentRes<Animator>())
		}
	);

public:
	AnimationFrameSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AnimationFrameSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
};