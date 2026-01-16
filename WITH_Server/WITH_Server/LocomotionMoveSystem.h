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
		return {  };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return {  };
	}

private:
	void ApplyNormalMovement(Entity entity, const LocomotionState& loco,
		Transform& trans, const Velocity& vel, const float dT);
};

