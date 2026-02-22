#pragma once

#include "System.h"
#include "Action.h"
#include "Animation.h"
#include "Movement.h"

class AnimationSelectSystem : public System {
public:
	AnimationSelectSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AnimationSelectSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(ActionState), typeid(LocomotionState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(AnimationState) };
	}
};