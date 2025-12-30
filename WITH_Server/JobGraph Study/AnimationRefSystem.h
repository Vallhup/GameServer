#pragma once

#include "ECS.h"
#include "System.h"

class AnimationRefSystem : public System {
public:
	AnimationRefSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationRefSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const;
	virtual std::vector<std::type_index> WriteComponents() const;
};