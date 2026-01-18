#pragma once

#include "ECS.h"
#include "System.h"

class ActionStateSystem : public System {
public:
	ActionStateSystem(ECS& e, int p = 0);
	virtual ~ActionStateSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const
	{
		return { typeid(ActionState) };
	}

	virtual std::vector<std::type_index> WriteComponents() const
	{
		return { typeid(ActionIntent), typeid(ActionRequestEvent) };
	}
};