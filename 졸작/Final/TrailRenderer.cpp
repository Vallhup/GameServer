#include "pch.h"
#include "TrailRenderer.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"

struct TrailConstants
{
	XMFLOAT4 color;
};

void TrailRenderer::Initialize(ID3D12Device* device, UINT maxPts)
{
	maxPoints = maxPts;
	points.reserve(maxPoints);
	vertices.reserve(maxPoints * 2);
	indices.reserve((maxPoints - 1) * 6);

	// 정점 버퍼 (최대 크기로 미리 할당)
	vertexBuffer = make_unique<UploadBuffer>();
	vertexBuffer->Initialize(device, maxPoints * 2 * sizeof(TrailVertex));

	// 인덱스 버퍼
	indexBuffer = make_unique<UploadBuffer>();
	indexBuffer->Initialize(device, (maxPoints - 1) * 6 * sizeof(UINT16));

	// 상수 버퍼
	trailCB = make_unique<UploadBuffer>();
	trailCB->Initialize(device, sizeof(TrailConstants));

	OutputDebugStringA("TrailRenderer initialized!\n");
}

void TrailRenderer::Update(float deltaTime)
{
	if (!isActive && points.empty()) return;

	// 각 포인트의 나이 증가
	for (auto& pt : points)
	{
		pt.age += deltaTime;
	}

	// 수명이 다한 포인트 제거 (앞에서부터)
	while (!points.empty() && points.front().age >= maxLifetime)
	{
		points.erase(points.begin());
		isDirty = true;
	}

	// 메시 재구성
	if (isDirty && !points.empty())
	{
		BuildMesh();
		isDirty = false;
	}
}

void TrailRenderer::AddPoint(const XMFLOAT3& top, const XMFLOAT3& bottom)
{
	if (!isActive) return;

	// 새 포인트 추가
	TrailPoint newPoint;
	newPoint.top = top;
	newPoint.bottom = bottom;
	newPoint.age = 0.0f;

	// 이전 포인트와 너무 가까우면 스킵 (최적화)
	if (!points.empty())
	{
		const auto& last = points.back();
		float dx = top.x - last.top.x;
		float dy = top.y - last.top.y;
		float dz = top.z - last.top.z;
		float distSq = dx * dx + dy * dy + dz * dz;

		if (distSq < 0.0001f) // 1cm 미만이면 스킵
			return;
	}

	// 최대 포인트 수 초과 시 가장 오래된 것 제거
	if (points.size() >= maxPoints)
	{
		points.erase(points.begin());
	}

	points.push_back(newPoint);
	isDirty = true;
}

void TrailRenderer::SetActive(bool active)
{
	isActive = active;

	if (!active)
	{
		// 비활성화 시 바로 지우지 않고 페이드 아웃 되도록 유지
	}
}

void TrailRenderer::Clear()
{
	points.clear();
	vertices.clear();
	indices.clear();
	isDirty = false;
}

void TrailRenderer::BuildMesh()
{
	if (points.size() < 2) return;

	vertices.clear();
	indices.clear();

	// 정점 생성: 각 포인트마다 top/bottom 두 개의 정점
	for (size_t i = 0; i < points.size(); ++i)
	{
		const auto& pt = points[i];
		float t = static_cast<float>(i) / static_cast<float>(points.size() - 1);
		float alpha = 1.0f - (pt.age / maxLifetime);	// 나이에 따라 페이드
		alpha = max(0.0f, alpha);

		// Top 정점
		TrailVertex topVert;
		topVert.position = pt.top;
		topVert.uv = { t, 0.0f };
		topVert.alpha = alpha;
		vertices.push_back(topVert);

		// Bottom 정점
		TrailVertex bottomVert;
		bottomVert.position = pt.bottom;
		bottomVert.uv = { t, 1.0f };
		bottomVert.alpha = alpha;
		vertices.push_back(bottomVert);
	}

	// 인덱스 생성: 쿼드를 삼각형 두 개로
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		UINT16 topLeft = static_cast<UINT16>(i * 2);
		UINT16 bottomLeft = static_cast<UINT16>(i * 2 + 1);
		UINT16 topRight = static_cast<UINT16>((i + 1) * 2);
		UINT16 bottomRight = static_cast<UINT16>((i + 1) * 2 + 1);

		// 첫 번째 삼각형
		indices.push_back(topLeft);
		indices.push_back(topRight);
		indices.push_back(bottomLeft);

		// 두 번째 삼각형
		indices.push_back(bottomLeft);
		indices.push_back(topRight);
		indices.push_back(bottomRight);
	}

	// 버퍼 업데이트
	if (!vertices.empty())
	{
		vertexBuffer->CopyData(vertices.data(), vertices.size() * sizeof(TrailVertex));

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(TrailVertex));
		vbView.StrideInBytes = sizeof(TrailVertex);
	}

	if (!indices.empty())
	{
		indexBuffer->CopyData(indices.data(), indices.size() * sizeof(UINT16));

		ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT16));
		ibView.Format = DXGI_FORMAT_R16_UINT;
	}
}

void TrailRenderer::Render(DX12Core& core)
{
	if (points.size() < 2 || vertices.empty() || indices.empty())
		return;

	auto cmdList = core.GetGraphicsCmdList();

	// 상수 버퍼 업데이트
	TrailConstants constants;
	constants.color = trailColor;
	trailCB->CopyData(&constants, sizeof(TrailConstants));

	// PSO 설정
	cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Trail));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());

	// FrameCB 바인딩 (b0 - View/Projection 행렬)
	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

	// TrailCB 바인딩 (루트 파라미터 인덱스 22 = b13 TrailCB)
	cmdList->SetGraphicsRootConstantBufferView(22, trailCB->GetGPUVirtualAddress());

	// 정점/인덱스 버퍼 바인딩
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	// 드로우
	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}
