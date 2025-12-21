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

	/*static bool wasZero = false;
	float yaw = camera->GetRadianYaw();
	if (inputX == 0 && inputZ == 0)
	{
		if (!wasZero)
		{
			input.SendMovePacket(0, 0, yaw, moveSeq++);
			wasZero = true;
		}
	}

	else
	{
		wasZero = false;
		input.SendMovePacket(inputX, inputZ, yaw, moveSeq++);
	}*/

	/*if (axisX == 0 && axisZ == 0)
	{
		if(!wasZero)
		{
			input.SendMovePacket(0, 0, yaw, moveSeq++);
			wasZero = true;
		}
	}

	else
	{
		wasZero = false;

		XMVECTOR forward = XMVectorSet(sin(yaw), 0, cos(yaw), 0);
		XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward);

		XMVECTOR dir =
			XMVectorAdd(XMVectorScale(forward, axisZ),
						XMVectorScale(right,   axisX));
		dir = XMVector3Normalize(dir);

		XMFLOAT3 d;
		XMStoreFloat3(&d, dir);

		input.SendMovePacket(d.x, d.z, yaw, moveSeq++);
	}*/

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

