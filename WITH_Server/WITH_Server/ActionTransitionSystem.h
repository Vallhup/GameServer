#pragma once

#include "ECS.h"
#include "System.h"

class ActionTransitionSystem : public System {
public:
	ActionTransitionSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ActionTransitionSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Velocity) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(ActionRequestTag), typeid(ActionMoveTag) };
	}

private:
	int GetPriority(ActionType type);
	float GetDuration(ActionType type);
	bool CanBeInterrupted(const ActionState& current, const ActionRequestTag& request);
	ActionType ResolveNextAction(const ActionState& current, const ActionRequestTag& request);
	void ApplyTransition(Entity entity, ActionState* state, ActionType next);
};

