#pragma once

#include "ECS.h"
#include "System.h"

class AnimationTimeSystem : public System {
public:
	AnimationTimeSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationTimeSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(AnimationRef), typeid(ActionState), 
			typeid(LocomotionState), typeid(LocomotionAnimPhase) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Animator) };
	}
};

