#include "pch.h"
#include "MainCharacter.h"
#include "Component.h"
#include "Transform.h"
#include "Input.h"
#include "Camera.h"
#include "AnimationMachine.h"
#include "Engine.h"

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
	auto& input = INPUT;

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

	if (auto* network = NETWORK_MANAGER)
	{
		network->SendMovePacket(inputX, inputZ, yaw, isRunning);
	}

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
	auto& input = INPUT;

	bool currentAttack = input.GetMouseButton(MouseButton::LEFT);

	if (currentAttack && !prevAttack) 
	{
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendAttackPacket(0.0f, 0.0f);
		}

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
	auto& input = INPUT;

	if (input.GetKeyDown(VK_SPACE))
	{
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendDodgePacket(0.0f, 0.0f);
		}

		auto animMachine = GetComponent<AnimationMachine>();
		if (animMachine)
		{
			animMachine->TryPlayClip("Dodge");
		}
	}
}

void MainCharacter::BasicGuard()
{
	auto& input = INPUT;

	bool isGuarding = input.GetKey('Q');

	auto animMachine = GetComponent<AnimationMachine>();
	if (animMachine) {
		if (!wasGuarding && isGuarding)
		{
			if (animMachine->TryPlayClip("Guard"))
			{
				if (auto* network = NETWORK_MANAGER)
				{
					network->SendGuardPacket(true);
				}
				wasGuarding = true;
			}
		}
		else if (wasGuarding && !isGuarding)
		{
			if (auto* network = NETWORK_MANAGER)
			{
				network->SendGuardPacket(false);
			}

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
	auto& input = INPUT;

	bool currentParry = input.GetMouseButton(MouseButton::RIGHT);

	if (currentParry && !prevParry) {
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendParryPacket(0.0f, 0.0f);
		}

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
		auto& input = INPUT;

		if (input.GetKey('Q')) 
		{
			if (auto* network = NETWORK_MANAGER)
			{
				network->SendGuardPacket(true);
			}
			wasGuarding = true;
			return "Guard";
		}

		if (input.GetMouseButton(MouseButton::RIGHT)) 
		{
			if (auto* network = NETWORK_MANAGER)
			{
				network->SendParryPacket(0.0f, 0.0f);
			}
			return "Parry";
		}

		if (input.GetKey(VK_SPACE)) 
		{
			if (auto* network = NETWORK_MANAGER)
			{
				network->SendDodgePacket(0.0f, 0.0f);
			}
			return "Dodge";
		}

		if (input.GetMouseButton(MouseButton::LEFT)) 
		{
			if (auto* network = NETWORK_MANAGER)
			{
				network->SendAttackPacket(0.0f, 0.0f);
			}
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

