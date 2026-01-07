#pragma once

#include "ECS.h"
#include "System.h"

class HitSystem : public System {
public:
	HitSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~HitSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const override;
	virtual std::vector<std::type_index> WriteComponents() const override;

private:
	void RequestActionTransition(Entity entity, ActionType type);
};