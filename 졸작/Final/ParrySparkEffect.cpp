#include "pch.h"
#include "ParrySparkEffect.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Material.h"

struct SparkConstants
{
	XMFLOAT4 color;
	UINT textureIndex;
	XMFLOAT3 padding;
};

void ParrySparkEffect::Initialize(ID3D12Device* device, UINT maxParts)
{
	maxParticles = maxParts;
	particles.reserve(maxParticles);
	vertices.reserve(maxParticles * 4);
	indices.reserve(maxParticles * 6);

	vertexBuffer = make_unique<UploadBuffer>();
	vertexBuffer->Initialize(device, maxParticles * 4 * sizeof(SparkVertex));

	indexBuffer = make_unique<UploadBuffer>();
	indexBuffer->Initialize(device, maxParticles * 6 * sizeof(UINT16));

	sparkCB = make_unique<UploadBuffer>();
	sparkCB->Initialize(device, sizeof(SparkConstants));
}

void ParrySparkEffect::Update(float deltaTime, const XMFLOAT3& cameraPos)
{
	if (particles.empty()) return;

	for (auto& p : particles)
	{
		p.age += deltaTime;

		p.position.x += p.velocity.x * deltaTime;
		p.position.y += p.velocity.y * deltaTime;
		p.position.z += p.velocity.z * deltaTime;

		p.velocity.y -= gravity * 0.5 * deltaTime;

		float drag = powf(0.004f, deltaTime);  
		p.velocity.x *= drag;
		p.velocity.y *= drag;
		p.velocity.z *= drag;

		float life = 1.0f - (p.age / maxLifetime);
		if (life < 0.0f) life = 0.0f;
		p.size = particleSize * life;
	}

	particles.erase(
		std::remove_if(particles.begin(), particles.end(),
			[this](const SparkParticle& p) { return p.age >= maxLifetime; }),
		particles.end()
	);

	if (!particles.empty())
	{
		BuildMesh(cameraPos);
	}
}

void ParrySparkEffect::SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
	if (device && cmdList) {
		textureIndex = Material::RegisterTexture(device, cmdList, path);
	}
}

void ParrySparkEffect::Clear()
{
	particles.clear();
	vertices.clear();
	indices.clear();
}

void ParrySparkEffect::Spawn(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (particles.size() >= maxParticles)
			break;

		SparkParticle p;
		p.position = position;

		float theTa = static_cast<float>(rand()) / RAND_MAX * XM_2PI;
		float phi = static_cast<float>(rand()) / RAND_MAX * XM_PI;  
		float speed = sparkSpeed * (0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f);

		p.velocity.x = sinf(phi) * cosf(theTa) * speed;
		p.velocity.y = cosf(phi) * speed;
		p.velocity.z = sinf(phi) * sinf(theTa) * speed;

		p.age = 0.0f;
		p.size = particleSize * (0.7f + static_cast<float>(rand()) / RAND_MAX * 0.3f);

		particles.push_back(p);
	}
}

void ParrySparkEffect::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);

	for (size_t i = 0; i < particles.size(); ++i)
	{
		const auto& p = particles[i];

		float lifeRatio = 1.0f - (p.age / maxLifetime);
		float alpha = max(0.0f, lifeRatio);

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
		vertexBuffer->CopyData(vertices.data(), vertices.size() * sizeof(SparkVertex));

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(SparkVertex));
		vbView.StrideInBytes = sizeof(SparkVertex);
	}

	if (!indices.empty())
	{
		indexBuffer->CopyData(indices.data(), indices.size() * sizeof(UINT16));

		ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT16));
		ibView.Format = DXGI_FORMAT_R16_UINT;
	}
}

void ParrySparkEffect::Render(DX12Core& core)
{
	if (particles.empty() || vertices.empty() || indices.empty())
		return;

	auto cmdList = core.GetGraphicsCmdList();

	SparkConstants constants;
	constants.color = sparkColor;
	constants.textureIndex = textureIndex;
	sparkCB->CopyData(&constants, sizeof(SparkConstants));

	cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Spark));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());

	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootConstantBufferView(23, sparkCB->GetGPUVirtualAddress());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}
