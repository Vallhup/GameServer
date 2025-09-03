#include "pch.h"
#include "MovementComponent.h"

MovementComponent::MovementComponent(GameObject& owner, Instance* instance) 
	: IComponent(owner, instance)
{
	_velocity = { 0.0f, 0.0f, 0.0f };
	_isRun = false;
}

void MovementComponent::Update(float deltaTime)
{
	if (_velocity == vec3{ 0.0f, 0.0f, 0.0f }) {
		return;
	}

	if (auto trComp = _owner.GetComponent<TransformComponent>()) {
		const float MOVE_SPEED = _isRun ? RUN_SPEED : WALK_SPEED;
		trComp->Translate(_velocity * MOVE_SPEED * deltaTime);
	}
}

void MovementComponent::SetMovePayload(const Protocol::InputPayload& payload)
{
	Protocol::Vec3 velocity = payload.move().velocity();
	_velocity = { velocity.x(), velocity.y(), velocity.z() };
	_isRun = payload.move().isrun();
}