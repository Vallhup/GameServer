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
	void ActionTransition(ActionState* state);
};

