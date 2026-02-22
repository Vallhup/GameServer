#pragma once

#include "System.h"
#include "Action.h"

class ActionTimeSystem : public System {
public:
	ActionTimeSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ActionTimeSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(ActionState) };
	}
};

