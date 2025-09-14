#pragma once

#include "GameObject.h"

class MovementComponent : public IComponent {
	static constexpr float WALK_SPEED{ 2.0f };
	static constexpr float RUN_SPEED{ 4.0f };

public:
	MovementComponent() = delete;
	MovementComponent(GameObject& owner, Instance* instance);
	virtual ~MovementComponent() = default;

	virtual void LogicUpdate(float deltaTime) override;

public:
	const vec3& GetDirection() const { return _direction; }

	void SetMovePayload(const Protocol::InputPayload& payload);

private:
	vec3 _direction;
	bool _isRun;
};