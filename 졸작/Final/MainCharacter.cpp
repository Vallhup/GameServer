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
	auto& input = GET(Input);

	static bool wasMoving = false;
	bool isMoving = ranges::any_of(
		initializer_list{ 'W', 'S', 'A', 'D' },
		[&input](int k) { return input.GetKey(k); }
	);

	// TEMP : 패킷 구조 어떻게 바뀌냐에 따라 달라짐
	bool dir[4]{ input.GetKey('W'), input.GetKey('S'), input.GetKey('D'), input.GetKey('A') };
	input.SendMovePacket(dir, camera->GetRadianYaw(), camera->GetRadianPitch());

	auto animator = GetComponent<Animator>();
	if (animator) {
		if (isMoving && !wasMoving) {
			animator->TransitionToAnimation(1, 0.3f);
			currentAnimState = 1;
		}
		else if (!isMoving && wasMoving) {
			animator->TransitionToAnimation(0, 0.3f);
			currentAnimState = 0;
		}
	}

	wasMoving = isMoving;
}

void MainCharacter::SetCamera(Camera* cam)
{
	camera = cam;

	camera->InitCameraPositionFromCharacter(GetComponent<Transform>()->GetPosition());
}
