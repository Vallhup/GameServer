#pragma once

#include "System.h"
#include "Movement.h"
#include "Stats.h"
#include "Animation.h"
#include "Action.h"

class LocomotionMoveSystem : public System {
public:
	LocomotionMoveSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~LocomotionMoveSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Velocity), typeid(LocomotionState), typeid(ActionState),
		typeid(FinalAttribute) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(LocomotionMoveDelta), typeid(LocomotionAnimPhase) };
	}

private:
	void ApplyNormalMovement(LocomotionMoveDelta* moveDelta,
		LocomotionAnimPhase* animPhase, const LocomotionState& loco, 
		const Velocity& vel, const FinalAttribute& attribute, const double dT);
};

