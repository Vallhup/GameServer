#include "pch.h"
#include "MainCharacter.h"
#include "Component.h"
#include "Transform.h"
#include "Input.h"
#include "Camera.h"
#include "Animator.h"

void MainCharacter::Update(float deltaTime)
{
	UpdateMovementDirections();
	BasicMove(deltaTime);

	GameObject::Update(deltaTime);

	// MainCharacter Update TODO
	camera->SetCameraPosition(GetComponent<Transform>()->GetPosition());
}

void MainCharacter::UpdateMovementDirections()
{
	if (!camera) return;

	characterForward = camera->GetForward();
	characterRight = camera->GetRight();

	characterForward.y = 0;
	characterRight.y = 0;

	XMStoreFloat3(&characterForward, XMVector3Normalize(XMLoadFloat3(&characterForward)));
	XMStoreFloat3(&characterRight, XMVector3Normalize(XMLoadFloat3(&characterRight)));
}

void MainCharacter::BasicMove(float deltaTime)
{
	auto transform = GetComponent<Transform>();
	if (!transform) return;

	XMFLOAT3 currentPos = transform->GetPosition();
	auto& input = GET(Input);

	static bool wasMoving = false;
	bool isMoving = false;
	XMFLOAT3 moveDirection = { 0, 0, 0 };

	if (input.GetKey('W'))
	{
		currentPos.x -= characterForward.x * MOVE_SPEED * deltaTime;
		currentPos.z -= characterForward.z * MOVE_SPEED * deltaTime;
		moveDirection.x -= characterForward.x;  
		moveDirection.z -= characterForward.z;
		isMoving = true;
	}
	if (input.GetKey('S'))
	{
		currentPos.x += characterForward.x * MOVE_SPEED * deltaTime;
		currentPos.z += characterForward.z * MOVE_SPEED * deltaTime;
		moveDirection.x += characterForward.x;
		moveDirection.z += characterForward.z;
		isMoving = true;
	}
	if (input.GetKey('A'))
	{
		currentPos.x += characterRight.x * MOVE_SPEED * deltaTime;
		currentPos.z += characterRight.z * MOVE_SPEED * deltaTime;
		moveDirection.x += characterRight.x;   
		moveDirection.z += characterRight.z;
		isMoving = true;
	}
	if (input.GetKey('D'))
	{
		currentPos.x -= characterRight.x * MOVE_SPEED * deltaTime;
		currentPos.z -= characterRight.z * MOVE_SPEED * deltaTime;
		moveDirection.x -= characterRight.x;
		moveDirection.z -= characterRight.z;
		isMoving = true;
	}
	transform->SetPosition(currentPos);

	XMFLOAT3 currentRot = transform->GetRotation();

	if (isMoving) {
		targetYawAngle = atan2(-moveDirection.x, -moveDirection.z);
		needsRotation = true;
	}

	if (needsRotation) {
		float angleDiff = targetYawAngle - currentYawAngle;

		while (angleDiff > XM_PI) angleDiff -= 2 * XM_PI;
		while (angleDiff < -XM_PI) angleDiff += 2 * XM_PI;

		currentYawAngle += angleDiff * ROT_SPEED * deltaTime;
		currentRot.y = currentYawAngle;

		if (abs(angleDiff) < 0.1f) needsRotation = false;
	}

	auto animator = GetComponent<Animator>();
	if (animator) {
		if (isMoving && !wasMoving) {
			animator->TransitionToAnimation(2, 0.3f);
			currentAnimState = 2;
		}
		else if (!isMoving && wasMoving) {
			animator->TransitionToAnimation(1, 0.3f);
			currentAnimState = 1;
		}
	}

	wasMoving = isMoving;
	transform->SetRotation(currentRot);
}

void MainCharacter::SetCamera(Camera* cam)
{
	camera = cam;

	camera->InitCameraPositionFromCharacter(GetComponent<Transform>()->GetPosition());
}
