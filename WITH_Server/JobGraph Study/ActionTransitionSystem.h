#pragma once

#include "ECS.h"
#include "System.h"

class ActionTransitionSystem : public System {
public:
	ActionTransitionSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ActionTransitionSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> WriteComponents() const;

private:
	int GetPriority(ActionType type);
	float GetDuration(ActionType type);
	ActionType ResolveNextAction(const ActionState& current, const ActionRequestTag& request);
	void ApplyTransition(ActionState* state, ActionType next);
};

