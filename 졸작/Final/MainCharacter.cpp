#include "pch.h"
#include "MainCharacter.h"
#include "Component.h"
#include "Transform.h"
#include "Input.h"
#include "Camera.h"
#include "AnimationMachine.h"

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

	auto animMachine = GetComponent<AnimationMachine>();
	if (animMachine) {
		if (!isMoving && wasMoving) {
			animMachine->TryPlayClip("Idle");
		}
		else if (isMoving && !wasMoving) {
			animMachine->TryPlayClip(isRunning ? "Run" : "Walk");
		}
		else if (isMoving && (isRunning != wasRunning)) {
			animMachine->TryPlayClip(isRunning ? "Run" : "Walk");
		}
	}

	wasMoving = isMoving;
	wasRunning = isRunning;
}

void MainCharacter::BasicAttack()
{
	auto& input = GET(Input);

	static bool prev{ false };
	bool now = input.GetMouseButton(MouseButton::LEFT);

	if (now && !prev) {
		input.SendAttackPacket();

		auto animMachine = GetComponent<AnimationMachine>();
		if (animMachine)
		{
			animMachine->TryPlayClip("Attack");
		}
	}

	prev = now;
}

void MainCharacter::BasicDodge()
{
	auto& input = GET(Input);

	if (input.GetKeyDown('C')) {
		input.SendDodgePacket();

		auto animMachine = GetComponent<AnimationMachine>();
		if (animMachine)
		{
			animMachine->TryPlayClip("Dodge");
		}
	}
}

void MainCharacter::RegisterAnimationCallback()
{
	auto animMachine = GetComponent<AnimationMachine>();
	if (!animMachine) return;

	animMachine->onActionEnd = [this]() -> string {
		auto& input = GET(Input);

		bool isMoving = input.GetKey('W') || input.GetKey('A') ||
						input.GetKey('S') || input.GetKey('D');

		bool isRunning = input.GetKey(VK_SHIFT) && isMoving;

		if (isRunning) return "Run";
		if (isMoving) return "Walk";
		return "Idle";
		};
}

void MainCharacter::SetAsLocalPlayer(Camera* cam)
{
	camera = cam;
	camera->InitCameraPositionFromCharacter(GetComponent<Transform>()->GetPosition());

	RegisterAnimationCallback();
}

