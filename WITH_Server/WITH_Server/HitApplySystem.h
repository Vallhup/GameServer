#pragma once

#include "ECS.h"
#include "System.h"

class HitApplySystem : public System {
public:
	HitApplySystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~HitApplySystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(HitTag), typeid(Health), typeid(ActionRequestTag) };
	}

private:
	void RequestActionTransition(Entity entity, ActionType type);
};