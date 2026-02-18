#pragma once

#include "System.h"
#include "Event.h"

class ActionTransitionSystem : public System {
public:
	ActionTransitionSystem(WorldRuntime& rt, int p = 0);
	virtual ~ActionTransitionSystem() = default;

	virtual void Execute(const double dT) override;
	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Velocity), typeid(ActionIntent) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(ActionMoveTag), typeid(ActionRequestEvent) };
	}

private:
	ActionType ResolveNextAction(const ActionState& current, ActionRequestEvent request, bool guardHeld, bool isForced);
	void ApplyTransition(Entity entity, ActionState* state, ActionType next);
	std::span<ActionRequestEvent> DedupActionRequest(std::span<ActionRequestEvent> events);
	

	void LoadTransitionRules();
	void SetRule(ActionType cur, ActionType req, ActionType next);
	ActionType GetRule(ActionType cur, ActionType req) const;

	static constexpr ActionType Invalid = static_cast<ActionType>(255);
	std::array<std::array<ActionType, ActionCount>, ActionCount> _transitionRules;
};

