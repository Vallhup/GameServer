#pragma once

#include "ECS.h"
#include "System.h"

class ActionTransitionSystem : public System {
public:
	ActionTransitionSystem(ECS& e, int p = 0);
	virtual ~ActionTransitionSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Velocity), typeid(ActionIntent) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(ActionMoveTag), typeid(ActionRequestEvent) };
	}

private:
	ActionType ResolveNextAction(const ActionState& current, ActionType request, bool guardHeld);
	void ApplyTransition(Entity entity, ActionState* state, ActionType next);
	void DedupActionRequest(std::vector<ActionRequestEvent>& events);

	void LoadTransitionRules();
	void SetRule(ActionType cur, ActionType req, ActionType next);
	ActionType GetRule(ActionType cur, ActionType req) const;

	static constexpr ActionType Invalid = static_cast<ActionType>(255);
	std::array<std::array<ActionType, ActionCount>, ActionCount> _transitionRules;
};

