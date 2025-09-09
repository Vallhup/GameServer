#include "pch.h"
#include "Transform.h"
#include "Input.h"

void Transform::Update(float deltaTime)
{
	//rotation.y += XM_PI * deltaTime;
	position.x += (targetPos.x - position.x) * deltaTime * 10.0f;
	position.y += (targetPos.y - position.y) * deltaTime * 10.0f;
	position.z += (targetPos.z - position.z) * deltaTime * 10.0f;
}

void Transform::SetPosition(float x, float y, float z)
{
	//position = { x, y, z };
	targetPos = { x, y, z };
}

void Transform::SetPosition(const XMFLOAT3& pos)
{
	//position = pos;
	targetPos = pos;
}

void Transform::SetInitPosition(float x, float y, float z)
{
	position = { x, y, z };
	targetPos = { x, y, z };
}

void Transform::SetRotation(float x, float y, float z)
{
	rotation = { x, y, z };
}

void Transform::SetRotation(const XMFLOAT3& rot)
{
	rotation = rot;
}

void Transform::SetScale(float x, float y, float z)
{
	scale = { x, y, z };
}

void Transform::SetScale(const XMFLOAT3& scl)
{
	scale = scl;
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
