#pragma once

#include "System.h"

class ActionMoveSystem : public System {
public:
	ActionMoveSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ActionMoveSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Velocity), typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(ActionMoveDelta), typeid(ActionMoveTag) };
	}

private:
	bool CanMove(ActionType type);
	void ApplyActionMovement(ActionMoveTag* actionMove, 
		ActionMoveDelta* actionDelta, const ActionState& actionState,
		const Velocity& vel, const double dT);
};