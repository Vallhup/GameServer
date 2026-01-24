#pragma once

#include "ECS.h"
#include "System.h"

class AnimationCommitSystem : public System {
public:
	AnimationCommitSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationCommitSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(AnimationState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Animator), typeid(CombatCollider) };
	}
	
private:
	void BindColliderToClip(CombatCollider* collider, const PrebakedAnimation& clip);
};
