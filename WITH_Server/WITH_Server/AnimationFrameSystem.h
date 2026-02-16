#pragma once

#include "System.h"

class AnimationFrameSystem : public System {
public:
	AnimationFrameSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AnimationFrameSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(ActionState), typeid(AnimationState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(Animator), typeid(LocomotionAnimPhase) };
	}
};

