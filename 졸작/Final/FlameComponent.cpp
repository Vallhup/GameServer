#include "pch.h"
#include "FlameComponent.h"
#include "Shader.h"
#include "Material.h"

PSOType FlameComponent::GetPSOType() const { return PSOType::Flame; }

void FlameComponent::Update(float deltaTime)
{
	if (particles.empty()) return;

	for (auto& p : particles)
	{
		p.age += deltaTime;
	}
}

void FlameComponent::Spawn(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (particles.size() >= maxElements)
			break;

		FlameParticle p;
		p.position = position;
		p.age = 0.0f;
		p.size = particleSize;
		particles.push_back(p);
	}
}

void FlameComponent::Clear()
{
	particles.clear();
	vertices.clear();
	indices.clear();
}

void FlameComponent::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList)
		textureIndex = Material::RegisterTexture(device, cmdList, path);
}

void FlameComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);

	for (size_t i = 0; i < particles.size(); ++i)
	{
		const auto& p = particles[i];

		float ageRatio = min(1.0f, p.age / fadeInTime);
		float alpha = ageRatio;

		auto randJitter = []() { return (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.0025f; };

		XMFLOAT3 jitteredPos = p.position;
		jitteredPos.x += randJitter();
		jitteredPos.y += randJitter();
		jitteredPos.z += randJitter();

		XMVECTOR particlePos = XMLoadFloat3(&jitteredPos);
		XMVECTOR toCamera = XMVectorSubtract(camPosVec, particlePos);
		toCamera = XMVector3Normalize(toCamera);

		XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR right = XMVector3Cross(worldUp, toCamera);
		right = XMVector3Normalize(right);
		XMVECTOR up = XMVector3Cross(toCamera, right);
		up = XMVector3Normalize(up);

		float halfSize = p.size * 0.5f;
		XMVECTOR rightScaled = XMVectorScale(right, halfSize);
		XMVECTOR upScaled = XMVectorScale(up, halfSize);

		XMFLOAT3 corners[4];
		XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(particlePos, rightScaled), upScaled));
		XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(particlePos, rightScaled), upScaled));
		XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(particlePos, rightScaled), upScaled));
		XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(particlePos, rightScaled), upScaled));

		UINT16 baseIdx = static_cast<UINT16>(vertices.size());

		vertices.push_back({ corners[0], {0.0f, 1.0f}, alpha });
		vertices.push_back({ corners[1], {1.0f, 1.0f}, alpha });
		vertices.push_back({ corners[2], {1.0f, 0.0f}, alpha });
		vertices.push_back({ corners[3], {0.0f, 0.0f}, alpha });

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 1);
		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 2);
	}
}
