#include "pch.h"
#include "FootDustEffect.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"

struct DustConstants
{
	XMFLOAT4 color;
};

void FootDustEffect::Initialize(ID3D12Device* device, UINT maxParts)
{
	maxParticles = maxParts;
	particles.reserve(maxParticles);
	vertices.reserve(maxParticles * 4);  
	indices.reserve(maxParticles * 6);   

	vertexBuffer = make_unique<UploadBuffer>();
	vertexBuffer->Initialize(device, maxParticles * 4 * sizeof(DustVertex));

	indexBuffer = make_unique<UploadBuffer>();
	indexBuffer->Initialize(device, maxParticles * 6 * sizeof(UINT16));

	dustCB = make_unique<UploadBuffer>();
	dustCB->Initialize(device, sizeof(DustConstants));
}

void FootDustEffect::Update(float deltaTime, const XMFLOAT3& cameraPos)
{
	if (particles.empty()) return;

	for (auto& p : particles)
	{
		p.age += deltaTime;

		p.position.x += p.velocity.x * deltaTime;
		p.position.z += p.velocity.z * deltaTime;

		float drag = powf(0.1f, deltaTime);  
		p.velocity.x *= drag;
		p.velocity.z *= drag;

		p.size += 0.02f * deltaTime;
	}

	particles.erase(
		std::remove_if(particles.begin(), particles.end(),
			[this](const DustParticle& p) { return p.age >= maxLifetime; }),
		particles.end()
	);

	if (!particles.empty())
	{
		BuildMesh(cameraPos);
	}
}

void FootDustEffect::Clear()
{
	particles.clear();
	vertices.clear();
	indices.clear();
	isDirty = false;
}

void FootDustEffect::Spawn(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (particles.size() >= maxParticles)
			break;

		DustParticle p;
		p.position = position;

		float angle = static_cast<float>(rand()) / RAND_MAX * XM_2PI;
		float speed = 0.02f + static_cast<float>(rand()) / RAND_MAX * 0.03f;
		p.velocity.x = cosf(angle) * speed;
		p.velocity.y = -0.05f;
		p.velocity.z = sinf(angle) * speed;

		p.age = 0.0f;
		p.size = particleSize * (0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f);

		particles.push_back(p);
	}

	isDirty = true;
}

void FootDustEffect::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);

	for (size_t i = 0; i < particles.size(); ++i)
	{
		const auto& p = particles[i];

		float alpha = 1.0f - (p.age / maxLifetime);
		alpha = max(0.0f, alpha);

		XMVECTOR particlePos = XMLoadFloat3(&p.position);
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

	if (!vertices.empty())
	{
		vertexBuffer->CopyData(vertices.data(), vertices.size() * sizeof(DustVertex));

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(DustVertex));
		vbView.StrideInBytes = sizeof(DustVertex);
	}

	if (!indices.empty())
	{
		indexBuffer->CopyData(indices.data(), indices.size() * sizeof(UINT16));

		ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT16));
		ibView.Format = DXGI_FORMAT_R16_UINT;
	}
}

void FootDustEffect::Render(DX12Core& core)
{
	if (particles.empty() || vertices.empty() || indices.empty())
		return;

	auto cmdList = core.GetGraphicsCmdList();

	DustConstants constants;
	constants.color = dustColor;
	dustCB->CopyData(&constants, sizeof(DustConstants));

	cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Trail));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());

	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootConstantBufferView(23, dustCB->GetGPUVirtualAddress());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}
