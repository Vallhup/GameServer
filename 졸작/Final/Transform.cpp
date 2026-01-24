#include "pch.h"
#include "Transform.h"
#include "Input.h"
#include "GameObject.h"

void Transform::Update(float deltaTime)
{
	position.x += (targetPos.x - position.x) * deltaTime * 10.0f;
	//position.y += (targetPos.y - position.y) * deltaTime * 10.0f;
	position.z += (targetPos.z - position.z) * deltaTime * 10.0f;

	float angleDiff = targetRot - rotation.y;

	while (angleDiff > XM_PI) angleDiff -= 2 * XM_PI;
	while (angleDiff < -XM_PI) angleDiff += 2 * XM_PI;

	rotation.y += angleDiff * deltaTime * 5.0f;

	UpdateBoundingBox();
}

void Transform::UpdateBoundingBox()
{
	if (!GetGameObject()) return;

	const BoundingBox& localBox = GetGameObject()->GetLocalBoundingBox();
	BoundingBox worldBox;

	XMMATRIX worldMatrix = GetWorldMatrix();
	localBox.Transform(worldBox, worldMatrix);

	GetGameObject()->SetWorldBoundingBox(worldBox);
}

void Transform::SetPosition(float x, float y, float z)
{
	targetPos = { x, y, z };
}

void Transform::SetPosition(const XMFLOAT3& pos)
{
	targetPos = pos;
}

void Transform::SetInitPosition(float x, float y, float z)
{
	position = { x, y, z };
	targetPos = { x, y, z };

	UpdateBoundingBox();
}

void Transform::SetInitPosition(const XMFLOAT3& pos)
{
	position = pos;
	targetPos = pos;

	UpdateBoundingBox();
}

void Transform::SetRotation(float x, float y, float z)
{
	targetRot = y;
	rotation = { x, y, z };

	UpdateBoundingBox();
}

void Transform::SetRotation(const XMFLOAT3& rot)
{
	targetRot = rot.y;
	rotation = rot;

	UpdateBoundingBox();
}

void Transform::SetTargetRotation(float y)
{
	targetRot = y;
}

void Transform::SetScale(float x, float y, float z)
{
	scale = { x, y, z };

	UpdateBoundingBox();
}

void Transform::SetScale(const XMFLOAT3& scl)
{
	scale = scl;

	UpdateBoundingBox();
}

void Transform::SetHeightImmediate(float y)
{
	position.y = y;
	targetPos.y = y;
}

const XMFLOAT3& Transform::GetPosition() const
{
	return position;
}

const XMFLOAT3& Transform::GetRotation() const
{
	return rotation;
}

const XMFLOAT3& Transform::GetScale() const
{
	return scale;
}

XMMATRIX Transform::GetWorldMatrix() const
{
	XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX R = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);

	return S * R * T;
}
