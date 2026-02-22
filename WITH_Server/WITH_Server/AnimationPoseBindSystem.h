#pragma once

#include "System.h"
#include "Animation.h"

class AnimationPoseBindSystem : public System {
public:
	AnimationPoseBindSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AnimationPoseBindSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Animator) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(CombatCollider) };
	}
};

