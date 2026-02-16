#pragma once
#include "System.h"

class MovementApplySystem : public System {
public:
	MovementApplySystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~MovementApplySystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(Transform),
			typeid(ActionMoveDelta), typeid(LocomotionMoveDelta) };
	}

private:
	void MovementApply(Entity entity, Transform* trans, 
		ActionMoveDelta* aDelta, LocomotionMoveDelta* lDelta, 
		const double dT);
};

