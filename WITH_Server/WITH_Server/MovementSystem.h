#pragma once

#include "System.h"
#include "Component.h"

class MovementSystem : public System {
public:
	MovementSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~MovementSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Velocity), typeid(LocomotionState), typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Transform), typeid(ActionMoveTag) };
	}

private:
	bool CanMove(ActionType type);
	void ApplyActionMovement(Entity entity, ActionMoveTag& action, Transform& trans, const Velocity& vel, const float dT);
	void ApplyNormalMovement(Entity entity, const LocomotionState& loco, Transform& trans, const Velocity& vel, const float dT);
};

