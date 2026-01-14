#pragma once

#include "ECS.h"
#include "System.h"

class AnimationTimeSystem : public System {
public:
	AnimationTimeSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationTimeSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const;
	virtual std::vector<std::type_index> WriteComponents() const;
};

