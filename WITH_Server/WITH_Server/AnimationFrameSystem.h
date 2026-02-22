#pragma once

#include "System.h"
#include "Action.h"
#include "Animation.h"

class AnimationFrameSystem : public System {
public:
	AnimationFrameSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AnimationFrameSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(ActionState), typeid(AnimationState), typeid(LocomotionAnimPhase) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(Animator) };
	}
};

