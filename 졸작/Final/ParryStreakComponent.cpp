#include "pch.h"
#include "ParryStreakComponent.h"
#include "Shader.h"
#include "Material.h"
#include "GameObject.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Transform.h"

PSOType ParryStreakComponent::GetPSOType() const { return PSOType::Glow; }

void ParryStreakComponent::Update(float deltaTime)
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

void ParryStreakComponent::Spawn(const XMFLOAT3& position)
{
	streakPos = position;
	age = 0.0f;
	alive = true;
}

void ParryStreakComponent::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList)
		textureIndex = Material::RegisterTexture(device, cmdList, path);
}

void ParryStreakComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	if (!alive) return;

	float t = age / maxLifetime;
	float alpha = (1.0f - t);
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;

	float widthScale = 0.3f + 0.7f * t;
	float heightScale = 1.0f - 0.5f * t;

	float halfW = streakWidth * 0.5f * widthScale;
	float halfH = streakHeight * 0.5f * heightScale;

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);
	XMVECTOR centerVec = XMLoadFloat3(&streakPos);
	XMVECTOR toCamera = XMVector3Normalize(XMVectorSubtract(camPosVec, centerVec));

	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, toCamera));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(toCamera, right));

	XMVECTOR rightScaled = XMVectorScale(right, halfW);
	XMVECTOR upScaled = XMVectorScale(up, halfH);

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
