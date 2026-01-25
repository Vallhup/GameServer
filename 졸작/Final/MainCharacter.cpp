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
		BasicGuard();
		BasicParry();
	}

	GameObject::Update(deltaTime);

	if (camera)
		camera->SetCameraPosition(GetComponent<Transform>()->GetPosition());
}

void MainCharacter::BasicMove()
{
	auto& input = GET(Input);

	bool isMoving = ranges::any_of(
		initializer_list{ 'W', 'S', 'A', 'D' },
		[&input](int k) { return input.GetKey(k); }
	);

	bool isRunning = input.GetKey(VK_SHIFT) && isMoving;

	int inputX{ 0 };
	int inputZ{ 0 };

	if (input.GetKey('W')) inputZ -= 1;
	if (input.GetKey('S')) inputZ += 1;
	if (input.GetKey('D')) inputX -= 1;
	if (input.GetKey('A')) inputX += 1;

	float yaw = camera->GetRadianYaw();
	input.SendMovePacket(inputX, inputZ, yaw, isRunning);

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

	bool currentAttack = input.GetMouseButton(MouseButton::LEFT);

	if (currentAttack && !prevAttack) {
		input.SendAttackPacket();

		auto animMachine = GetComponent<AnimationMachine>();
		if (animMachine)
		{
			animMachine->TryPlayClip("Attack");
		}
	}

	prevAttack = currentAttack;
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

void MainCharacter::BasicGuard()
{
	auto& input = GET(Input);

	bool isGuarding = input.GetKey('Q');

	auto animMachine = GetComponent<AnimationMachine>();
	if (animMachine) {
		if (!wasGuarding && isGuarding)
		{
			if (animMachine->TryPlayClip("Guard"))
			{
				input.SendGuardPacket(true);
				wasGuarding = true;
			}
		}
		else if (wasGuarding && !isGuarding)
		{
			input.SendGuardPacket(false);

			if (animMachine->IsPlaying("Guard"))
			{
				animMachine->EndCurrentClip();
			}
			wasGuarding = false;
		}
	}
}

void MainCharacter::BasicParry()
{
	auto& input = GET(Input);

	bool currentParry = input.GetMouseButton(MouseButton::RIGHT);

	if (currentParry && !prevParry) {
		input.SendParryPacket(true);

		auto animMachine = GetComponent<AnimationMachine>();
		if (animMachine)
		{
			animMachine->TryPlayClip("Parry");
		}
	}

	prevParry = currentParry;
}

void MainCharacter::RegisterAnimationCallback()
{
	auto animMachine = GetComponent<AnimationMachine>();
	if (!animMachine) return;

	animMachine->onActionEnd = [this]() -> string {
		auto& input = GET(Input);

		if (input.GetKey('Q')) {
			input.SendGuardPacket(true);
			wasGuarding = true;
			return "Guard";
		}

		if (input.GetMouseButton(MouseButton::RIGHT)) {
			input.SendParryPacket(true);
			return "Parry";
		}

		if (input.GetKey('C')) {
			input.SendDodgePacket();
			return "Dodge";
		}

		if (input.GetMouseButton(MouseButton::LEFT)) {
			input.SendAttackPacket();
			return "Attack";
		}

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

