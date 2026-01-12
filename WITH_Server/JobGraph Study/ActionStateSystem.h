#pragma once

#include "ECS.h"
#include "System.h"

class ActionStateSystem : public System {
public:
	ActionStateSystem(ECS& e, int p = 0);
	virtual ~ActionStateSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const;
	virtual std::vector<std::type_index> WriteComponents() const;

private:
	ActionType GetNextAction(const ActionState& current, const ActionIntent& intent);
	bool StartAction(Entity entity, ActionState* state, const ActionType& type);
	void ResetActionIntent(ActionIntent* intent);
};