#include "pch.h"
#include "MainCharacter.h"
#include "Component.h"
#include "Transform.h"
#include "Input.h"
#include "Camera.h"
#include "Engine.h"
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
		BasicDrinking();
	}

	GameObject::Update(deltaTime);

	if (camera)
		camera->SetCameraPosition(GetComponent<Transform>()->GetPosition());
}

void MainCharacter::BasicMove()
{
	auto& input = INPUT;

	int inputX{ 0 };
	int inputZ{ 0 };

	if (input.GetKey('W')) inputZ -= 1;
	if (input.GetKey('S')) inputZ += 1;
	if (input.GetKey('D')) inputX -= 1;
	if (input.GetKey('A')) inputX += 1;

	const bool isRunning =
		input.GetKey(VK_SHIFT) &&
		(inputX != 0 || inputZ != 0);

	const float yaw = camera->GetRadianYaw();

	if (auto* network = NETWORK_MANAGER)
	{
		network->SendMovePacket(inputX, inputZ, yaw, isRunning);
	}
}

void MainCharacter::BasicAttack()
{
	auto& input = INPUT;

	bool currentAttack = input.GetMouseButton(MouseButton::LEFT);

	if (currentAttack && !prevAttack)
	{
		if (auto* network = NETWORK_MANAGER)
		{
			uint32_t animId = 0;
			uint32_t instanceId = 0;
			float normalizedTime = 0.0f;

			if (auto* animMachine = GetComponent<AnimationMachine>())
			{
				if (animMachine->HasServerAbilityTiming())
				{
					animId = animMachine->GetServerAnimId();
					instanceId = animMachine->GetAbilityInstanceId();
					normalizedTime = animMachine->GetCurrentNormalizedTime();
				}
			}

			network->SendAttackPacket(
				0.0f,
				0.0f,
				animId,
				normalizedTime,
				instanceId);
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
	}
}

void MainCharacter::BasicGuard()
{
	auto& input = INPUT;

	bool isGuarding = input.GetKey('Q');

	if (!wasGuarding && isGuarding)
	{
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendGuardPacket(true);
		}
		wasGuarding = true;
	}
	else if (wasGuarding && !isGuarding)
	{
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendGuardPacket(false);
		}
		wasGuarding = false;
	}
}

void MainCharacter::BasicParry()
{
	auto& input = INPUT;

	bool currentParry = input.GetMouseButton(MouseButton::RIGHT);

	if (currentParry && !prevParry)
	{
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendParryPacket(0.0f, 0.0f);
		}
	}

	prevParry = currentParry;
}

void MainCharacter::BasicDrinking()
{
	auto& input = INPUT;

	if (input.GetKeyDown('1'))
	{
		if (auto* network = NETWORK_MANAGER)
		{
			network->SendUseItemPacket(0, 0);
		}
	}
}

void MainCharacter::SetAsLocalPlayer(Camera* cam)
{
	camera = cam;
	camera->InitCameraPositionFromCharacter(GetComponent<Transform>()->GetPosition());
}

