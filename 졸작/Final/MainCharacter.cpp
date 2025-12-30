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
	static bool wasRunning = false;

	bool isMoving = ranges::any_of(
		initializer_list{ 'W', 'S', 'A', 'D' },
		[&input](int k) { return input.GetKey(k); }
	);

	bool isRunning = input.GetKey(VK_SHIFT) && isMoving;

	int inputX{ 0 };
	int inputZ{ 0 };

	if (input.GetKey('W')) inputZ -= isRunning ? 2 : 1;
	if (input.GetKey('S')) inputZ += isRunning ? 2 : 1;
	if (input.GetKey('D')) inputX -= isRunning ? 2 : 1;
	if (input.GetKey('A')) inputX += isRunning ? 2 : 1;

	float yaw = camera->GetRadianYaw();
	input.SendMovePacket(inputX, inputZ, yaw);

	auto animator = GetComponent<Animator>();
	if (animator) {
		if (!isMoving && wasMoving) {
			animator->TransitionToAnimation(0, 0.3f);
			currentAnimState = 0;
		}
		else if (isMoving && !wasMoving) {
			int anim = isRunning ? 1 : 2;
			animator->TransitionToAnimation(anim, 0.3f);
			currentAnimState = anim;
		}
		else if (isMoving && (isRunning != wasRunning)) {
			int anim = isRunning ? 1 : 2;
			animator->TransitionToAnimation(anim, 0.3f);
			currentAnimState = anim;
		}
	}

	wasMoving = isMoving;
	wasRunning = isRunning;
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

