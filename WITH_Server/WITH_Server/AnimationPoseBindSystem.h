#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Animation.h"

class AnimationPoseBindSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AnimationPoseBindSystem>(),
		"AnimationPoseBindSystem",
		std::array{
			ReadImmediate(ComponentRes<Animator>()),
			WriteImmediate(ComponentRes<CombatCollider>())
		}
	);

public:
	AnimationPoseBindSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AnimationPoseBindSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
};

