#pragma once

#include "ECS.h"
#include "System.h"

class HitResolveSystem : public System {
public:
	HitResolveSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~HitResolveSystem() {}

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const override;
	virtual std::vector<std::type_index> WriteComponents() const override;
};

