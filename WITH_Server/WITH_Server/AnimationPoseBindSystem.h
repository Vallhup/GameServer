#pragma once

#include "ECS.h"
#include "System.h"

class AnimationPoseBindSystem : public System {
public:
	AnimationPoseBindSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationPoseBindSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Animator) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Collider) };
	}
};

