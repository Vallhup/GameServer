#pragma once

#include "ECS.h"
#include "System.h"

class ViewProcessingSystem : public System {
public:
	ViewProcessingSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ViewProcessingSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const override
	{
		return { typeid(Transform) };
	}

	virtual std::vector<std::type_index> WriteComponents() const override
	{
		return { typeid(ViewList) };
	}
};