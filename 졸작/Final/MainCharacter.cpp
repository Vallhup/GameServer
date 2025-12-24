#include "pch.h"
#include "MainCharacter.h"
#include "Component.h"
#include "Transform.h"
#include "Input.h"
#include "Camera.h"
#include "Animator.h"

void MainCharacter::Update(float deltaTime)
{
	if (camera)
	{
		BasicMove();
		BasicAttack();
		BasicDodge();
	}

	GameObject::Update(deltaTime);

	if (camera)
		camera->SetCameraPosition(GetComponent<Transform>()->GetPosition());
}

void MainCharacter::BasicMove()
{
	auto& input = GET(Input);

	static bool wasMoving = false;
	bool isMoving = ranges::any_of(
		initializer_list{ 'W', 'S', 'A', 'D' },
		[&input](int k) { return input.GetKey(k); }
	);

	int inputX{ 0 };
	int inputZ{ 0 };

	if (input.GetKey('W')) inputZ -= 1;
	if (input.GetKey('S')) inputZ += 1;
	if (input.GetKey('D')) inputX -= 1;
	if (input.GetKey('A')) inputX += 1;

	float yaw = camera->GetRadianYaw();
	input.SendMovePacket(inputX, inputZ, yaw);

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

void MainCharacter::BasicAttack()
{
	auto& input = GET(Input);

	if (input.GetMouseButton(MouseButton::LEFT)) {
		input.SendAttackPacket();

		auto animator = GetComponent<Animator>();
		if (animator) {
			// TODO : Attack Animation
		}
	}
}

void MainCharacter::BasicDodge()
{
	auto& input = GET(Input);

	if (input.GetKey(VK_SHIFT)) {
		input.SendDodgePacket();

		auto animator = GetComponent<Animator>();
		if (animator) {
			// TODO : Dodge Animation
		}
	}
}

void MainCharacter::SetCamera(Camera* cam)
{
	camera = cam;

	camera->InitCameraPositionFromCharacter(GetComponent<Transform>()->GetPosition());
}

