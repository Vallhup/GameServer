#include "pch.h"
#include "BeaconLightComponent.h"
#include "Shader.h"
#include "Material.h"

PSOType BeaconLightComponent::GetPSOType() const { return PSOType::Glow; }

void BeaconLightComponent::Update(float deltaTime)
{
	if (!alive) return;
	age += deltaTime;
}

void BeaconLightComponent::Spawn(const XMFLOAT3& position)
{
	beaconPos = position;
	age = 0.0f;
	alive = true;
}

void BeaconLightComponent::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList)
		textureIndex = Material::RegisterTexture(device, cmdList, path);
}

void BeaconLightComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	if (!alive) return;

	const float pulse = PULSE_BASE + PULSE_DEPTH * sinf(age * PULSE_SPEED);
	const float alpha = pulse;
	const float halfSize = beaconSize * 0.5f * pulse;

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);
	XMVECTOR centerVec = XMLoadFloat3(&beaconPos);
	XMVECTOR toCamera = XMVector3Normalize(XMVectorSubtract(camPosVec, centerVec));

	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, toCamera));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(toCamera, right));

	const float spins[2] = { 0.0f, XM_PIDIV4 };
	for (int q = 0; q < 2; ++q)
	{
		const float cs = cosf(spins[q]);
		const float sn = sinf(spins[q]);
		XMVECTOR rRot = XMVectorAdd(XMVectorScale(right, cs), XMVectorScale(up, sn));
		XMVECTOR uRot = XMVectorAdd(XMVectorScale(right, -sn), XMVectorScale(up, cs));

		XMVECTOR rightScaled = XMVectorScale(rRot, halfSize);
		XMVECTOR upScaled = XMVectorScale(uRot, halfSize);

		XMFLOAT3 corners[4];
		XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(centerVec, rightScaled), upScaled));
		XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(centerVec, rightScaled), upScaled));
		XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(centerVec, rightScaled), upScaled));
		XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(centerVec, rightScaled), upScaled));

		const UINT16 base = static_cast<UINT16>(vertices.size());
		vertices.push_back({ corners[0], {0.0f, 1.0f}, alpha });
		vertices.push_back({ corners[1], {1.0f, 1.0f}, alpha });
		vertices.push_back({ corners[2], {1.0f, 0.0f}, alpha });
		vertices.push_back({ corners[3], {0.0f, 0.0f}, alpha });

		indices.push_back(base + 0);
		indices.push_back(base + 2);
		indices.push_back(base + 1);
		indices.push_back(base + 0);
		indices.push_back(base + 3);
		indices.push_back(base + 2);
	}
}
