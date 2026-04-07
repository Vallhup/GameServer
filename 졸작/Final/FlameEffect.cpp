#include "pch.h"
#include "FlameEffect.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Material.h"

struct FlameConstants
{
	XMFLOAT4 color;
	UINT textureIndex;
	XMFLOAT3 padding;
};

void FlameEffect::Initialize(ID3D12Device* device, UINT maxParts)
{
	maxParticles = maxParts;
	particles.reserve(maxParticles);
	vertices.reserve(maxParticles * 4);
	indices.reserve(maxParticles * 6);

	vertexBuffer = make_unique<UploadBuffer>();
	vertexBuffer->Initialize(device, maxParticles * 4 * sizeof(FlameVertex));

	indexBuffer = make_unique<UploadBuffer>();
	indexBuffer->Initialize(device, maxParticles * 6 * sizeof(UINT16));

	flameCB = make_unique<UploadBuffer>();
	flameCB->Initialize(device, sizeof(FlameConstants));
}

void FlameEffect::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList) {
		textureIndex = Material::RegisterTexture(device, cmdList, path);
	}
}

void FlameEffect::Update(float deltaTime, const XMFLOAT3& cameraPos)
{
	if (particles.empty()) return;

	for (auto& p : particles)
	{
		p.age += deltaTime;
	}

	BuildMesh(cameraPos);
}

void FlameEffect::Clear()
{
	particles.clear();
	vertices.clear();
	indices.clear();
}

void FlameEffect::Spawn(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (particles.size() >= maxParticles)
			break;

		FlameParticle p;
		p.position = position;

		p.age = 0.0f;
		p.size = particleSize;

		particles.push_back(p);
	}
}

void FlameEffect::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);

	for (size_t i = 0; i < particles.size(); ++i)
	{
		const auto& p = particles[i];

		float ageRatio = min(1.0f, p.age / fadeInTime);
		float alpha = ageRatio;

		auto randJitter = []() { return (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.001f; };
		
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
		XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(particlePos, rightScaled), upScaled)); // 좌하
		XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(particlePos, rightScaled), upScaled));      // 우하
		XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(particlePos, rightScaled), upScaled));           // 우상
		XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(particlePos, rightScaled), upScaled));      // 좌상

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

	if (!vertices.empty())
	{
		vertexBuffer->CopyData(vertices.data(), vertices.size() * sizeof(FlameVertex));

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(FlameVertex));
		vbView.StrideInBytes = sizeof(FlameVertex);
	}

	if (!indices.empty())
	{
		indexBuffer->CopyData(indices.data(), indices.size() * sizeof(UINT16));

		ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT16));
		ibView.Format = DXGI_FORMAT_R16_UINT;
	}
}

void FlameEffect::Render(DX12Core& core)
{
	if (particles.empty() || vertices.empty() || indices.empty())
		return;

	auto cmdList = core.GetGraphicsCmdList();

	FlameConstants constants;
	constants.color = flameColor;
	constants.textureIndex = textureIndex;
	flameCB->CopyData(&constants, sizeof(FlameConstants));

	cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Flame));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());

	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootConstantBufferView(23, flameCB->GetGPUVirtualAddress());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}
