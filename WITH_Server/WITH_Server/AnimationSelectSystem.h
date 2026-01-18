#pragma once

#include "ECS.h"
#include "System.h"

class AnimationSelectSystem : public System {
public:
	AnimationSelectSystem(ECS& e, int p = 0);
	virtual ~AnimationSelectSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(ActionState), typeid(LocomotionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(AnimationState) };
	}

private:
	std::pair<AnimationId, bool> GetAnimationIdForAction(ActionType action) const;

	std::unordered_map<ActionType, std::pair<AnimationId, bool>> 
		_actionToAnimationMap;
};