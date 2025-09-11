#include "pch.h"
#include "MovementComponent.h"

MovementComponent::MovementComponent(GameObject& owner, Instance* instance) 
	: IComponent(owner, instance)
{
	_direction = { 0.0f, 0.0f, 0.0f };
	_isRun = false;
}

void MovementComponent::LogicUpdate(float deltaTime)
{
	if (_direction == vec3{ 0.0f, 0.0f, 0.0f }) {
		return;
	}

	if (auto trComp = _owner.GetComponent<TransformComponent>()) {
		const float MOVE_SPEED = _isRun ? RUN_SPEED : WALK_SPEED;
		trComp->Translate(_direction * MOVE_SPEED * deltaTime);
	}
}

void MovementComponent::SetMovePayload(const Protocol::InputPayload& payload)
{
	float yaw = payload.move().camyaw();
	float pitch = payload.move().campitch();

	vec3 forward = vec3{
		cos(pitch) * sin(yaw),
		sin(pitch),
		cos(pitch) * cos(yaw)
	}.Normalize();
	vec3 up{ 0.0f, 1.0f, 0.0f };
	vec3 right = up.Cross(forward);

	vec3 direction{ 0.0f, 0.0f, 0.0f };

	if (payload.move().front()) {
		direction.x -= forward.x;
		direction.z -= forward.z;
	}

	if (payload.move().back()) {
		direction.x += forward.x;
		direction.z += forward.z;
	}

	if (payload.move().right()) {
		direction.x -= right.x;
		direction.z -= right.z;
	}

	if (payload.move().left()) {
		direction.x += right.x;
		direction.z += right.z;
	}

	_direction = direction;
	_isRun = payload.move().isrun();
}