#pragma once

#include "ECS.h"
#include "System.h"

class AnimationRefSystem : public System {
public:
	AnimationRefSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationRefSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(AnimationRef), typeid(Animator) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Collider) };
	}
};