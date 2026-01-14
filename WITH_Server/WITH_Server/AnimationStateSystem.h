#pragma once

#include "ECS.h"
#include "System.h"

class AnimationStateSystem : public System {
public:
	AnimationStateSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationStateSystem() = default;

	virtual void Execute(const float dT) override;
	virtual std::vector<std::type_index> ReadComponents() const;
	virtual std::vector<std::type_index> WriteComponents() const;

private:
	void ChangeAnimation(Entity e, AnimationId id);
};

