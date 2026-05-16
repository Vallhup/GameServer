#include "pch.h"
#include "ParryFlashComponent.h"
#include "Shader.h"
#include "Material.h"
#include "GameObject.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Transform.h"

PSOType ParryFlashComponent::GetPSOType() const { return PSOType::Glow; }

void ParryFlashComponent::Update(float deltaTime)
{
	if (!alive) return;

	age += deltaTime;
	if (age >= maxLifetime)
	{
		alive = false;
		vertices.clear();
		indices.clear();
	}
}

void ParryFlashComponent::Spawn(const XMFLOAT3& position)
{
	flashPos = position;
	age = 0.0f;
	alive = true;
}

void ParryFlashComponent::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList)
		textureIndex = Material::RegisterTexture(device, cmdList, path);
}

void ParryFlashComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	if (!alive) return;

	float t = age / maxLifetime;
	float alpha = (1.0f - t);
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;

	float ease = 1.0f - (1.0f - t) * (1.0f - t);
	float scale = 0.4f + 2.0f * ease;
	float halfSize = flashSize * 0.5f * scale;

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);
	XMVECTOR centerVec = XMLoadFloat3(&flashPos);
	XMVECTOR toCamera = XMVectorSubtract(camPosVec, centerVec);
	toCamera = XMVector3Normalize(toCamera);

	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Cross(worldUp, toCamera);
	right = XMVector3Normalize(right);
	XMVECTOR up = XMVector3Cross(toCamera, right);
	up = XMVector3Normalize(up);

	XMVECTOR rightScaled = XMVectorScale(right, halfSize);
	XMVECTOR upScaled = XMVectorScale(up, halfSize);

	XMFLOAT3 corners[4];
	XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(centerVec, rightScaled), upScaled));
	XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(centerVec, rightScaled), upScaled));
	XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(centerVec, rightScaled), upScaled));
	XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(centerVec, rightScaled), upScaled));

	vertices.push_back({ corners[0], {0.0f, 1.0f}, alpha });
	vertices.push_back({ corners[1], {1.0f, 1.0f}, alpha });
	vertices.push_back({ corners[2], {1.0f, 0.0f}, alpha });
	vertices.push_back({ corners[3], {0.0f, 0.0f}, alpha });

	indices.push_back(0);
	indices.push_back(2);
	indices.push_back(1);
	indices.push_back(0);
	indices.push_back(3);
	indices.push_back(2);
}
