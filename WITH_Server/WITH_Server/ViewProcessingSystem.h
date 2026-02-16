#pragma once

#include "System.h"

class ViewProcessingSystem : public System {
public:
	ViewProcessingSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ViewProcessingSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Transform) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(ViewList) };
	}
};