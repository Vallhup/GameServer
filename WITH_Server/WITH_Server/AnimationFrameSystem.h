#pragma once

#include "ECS.h"
#include "System.h"

class AnimationFrameSystem : public System {
public:
	AnimationFrameSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationFrameSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(ActionState), typeid(AnimationState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Animator), typeid(LocomotionAnimPhase) };
	}
};

