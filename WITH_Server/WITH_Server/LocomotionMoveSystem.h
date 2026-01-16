#pragma once

#include "ECS.h"
#include "System.h"

class LocomotionMoveSystem : public System {
public:
	LocomotionMoveSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~LocomotionMoveSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Velocity), typeid(LocomotionState),
		typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(LocomotionMoveDelta) };
	}

private:
	void ApplyNormalMovement(LocomotionMoveDelta* moveDelta,
		LocomotionAnimPhase* animPhase, const LocomotionState& loco, 
		const Velocity& vel, const float dT);
};

