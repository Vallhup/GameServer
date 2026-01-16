#pragma once

#include "ECS.h"
#include "System.h"

class MovementApplySystem : public System {
public:
	MovementApplySystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~MovementApplySystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(Transform),
			typeid(ActionMoveDelta), typeid(LocomotionMoveDelta) };
	}

private:
	void MovementApply(Entity entity, Transform* trans, 
		ActionMoveDelta* aDelta, LocomotionMoveDelta* lDelta, 
		const float dT);
};

