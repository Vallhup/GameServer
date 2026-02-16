#pragma once

#include "System.h"

class AnimationCommitSystem : public System {
public:
	AnimationCommitSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AnimationCommitSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(AnimationState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(Animator), typeid(CombatCollider) };
	}
	
private:
	void BindColliderToClip(CombatCollider* collider, const PrebakedAnimation& clip);
};
