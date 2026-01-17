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
		return { typeid(ActionMoveTag) };
	}

private:
	int GetPriority(ActionType type);
	float GetDuration(ActionType type);
	bool CanBeInterrupted(const ActionState& current, ActionType request);
	ActionType ResolveNextAction(const ActionState& current, ActionType request);
	void ApplyTransition(Entity entity, ActionState* state, ActionType next);
	void DedupActionRequest(std::vector<ActionRequestEvent>& events);
};

