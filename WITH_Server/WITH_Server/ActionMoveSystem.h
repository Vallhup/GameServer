#pragma once

#include "ECS.h"
#include "System.h"

class ActionMoveSystem : public System {
public:
	ActionMoveSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ActionMoveSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Velocity), typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(ActionMoveDelta), typeid(ActionMoveTag) };
	}

private:
	bool CanMove(ActionType type);
	void ApplyActionMovement(ActionMoveTag* actionMove, 
		ActionMoveDelta* actionDelta, const ActionState& actionState,
		const Velocity& vel, const float dT);
};