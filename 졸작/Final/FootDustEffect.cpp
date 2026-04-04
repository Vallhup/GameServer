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
	vertices.reserve(maxParticles * 4);  // 파티클당 4개 정점 (쿼드)
	indices.reserve(maxParticles * 6);   // 파티클당 6개 인덱스 (삼각형 2개)

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

		// 아주 느리게 수평으로만 퍼짐
		p.position.x += p.velocity.x * deltaTime;
		p.position.z += p.velocity.z * deltaTime;

		// 감속
		p.velocity.x *= 0.9f;
		p.velocity.z *= 0.9f;

		// 크기 살짝 증가
		p.size += 0.02f * deltaTime;
	}

	// 수명 다한 파티클 제거
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

		// 아주 느린 수평 퍼짐
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

		// 알파: 나이에 따라 페이드 아웃
		float alpha = 1.0f - (p.age / maxLifetime);
		alpha = max(0.0f, alpha);

		// 빌보드 계산: 카메라를 향하는 쿼드
		XMVECTOR particlePos = XMLoadFloat3(&p.position);
		XMVECTOR toCamera = XMVectorSubtract(camPosVec, particlePos);
		toCamera = XMVector3Normalize(toCamera);

		// 월드 업 벡터
		XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		// 빌보드의 오른쪽/위쪽 벡터 계산
		XMVECTOR right = XMVector3Cross(worldUp, toCamera);
		right = XMVector3Normalize(right);
		XMVECTOR up = XMVector3Cross(toCamera, right);
		up = XMVector3Normalize(up);

		float halfSize = p.size * 0.5f;
		XMVECTOR rightScaled = XMVectorScale(right, halfSize);
		XMVECTOR upScaled = XMVectorScale(up, halfSize);

		// 4개 코너 정점 생성
		XMFLOAT3 corners[4];
		XMStoreFloat3(&corners[0], XMVectorSubtract(XMVectorSubtract(particlePos, rightScaled), upScaled)); // 좌하
		XMStoreFloat3(&corners[1], XMVectorSubtract(XMVectorAdd(particlePos, rightScaled), upScaled));      // 우하
		XMStoreFloat3(&corners[2], XMVectorAdd(XMVectorAdd(particlePos, rightScaled), upScaled));           // 우상
		XMStoreFloat3(&corners[3], XMVectorAdd(XMVectorSubtract(particlePos, rightScaled), upScaled));      // 좌상

		UINT16 baseIdx = static_cast<UINT16>(vertices.size());

		// 정점 추가
		vertices.push_back({ corners[0], {0.0f, 1.0f}, alpha });
		vertices.push_back({ corners[1], {1.0f, 1.0f}, alpha });
		vertices.push_back({ corners[2], {1.0f, 0.0f}, alpha });
		vertices.push_back({ corners[3], {0.0f, 0.0f}, alpha });

		// 인덱스 추가 (삼각형 2개)
		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 1);

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 2);
	}

	// 버퍼 업데이트
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

	// 상수 버퍼 업데이트
	DustConstants constants;
	constants.color = dustColor;
	dustCB->CopyData(&constants, sizeof(DustConstants));

	// Trail PSO 재사용 (Additive blending)
	cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Trail));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());

	// FrameCB (b0)
	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

	// TrailCB (b13) - 색상용
	cmdList->SetGraphicsRootConstantBufferView(23, dustCB->GetGPUVirtualAddress());

	// 렌더링
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}
