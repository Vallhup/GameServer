#include "pch.h"
#include "ShadowMappingManager.h"
#include "LightManager.h"

void ShadowMappingManager::Initialize(ID3D12Device* device)
{
	Invalidate();
	SettingsForCSM();
	CreateCSMResources(device);
	CreateStaticCSMResources(device);
	CreateAtlasResources();
}

void ShadowMappingManager::UpdateCascadeShadow(const XMFLOAT3& center)
{
	XMFLOAT3 currentSunDir = {};
	if (lightMgr)
	{
		// lights[0] = sun (directional). position 필드가 direction 역할.
		XMFLOAT3 dir = lightMgr->GetLights()[0].position;
		csmLightDir = XMVector3Normalize(XMLoadFloat3(&dir));
		XMStoreFloat3(&currentSunDir, csmLightDir);
	}

	bool sunChanged =
		(currentSunDir.x != lastSunDir.x) ||
		(currentSunDir.y != lastSunDir.y) ||
		(currentSunDir.z != lastSunDir.z);

	XMVECTOR centerPos = XMLoadFloat3(&center);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	const float shadowCasterDistance = 110.0f;

	// centerPos에 의존하지 않는 라이트 공간 축 (shadow swimming 방지용)
	XMVECTOR forward = XMVector3Normalize(csmLightDir);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
	XMVECTOR trueUp = XMVector3Normalize(XMVector3Cross(forward, right));

	float cx = XMVectorGetX(XMVector3Dot(centerPos, right));
	float cy = XMVectorGetX(XMVector3Dot(centerPos, trueUp));
	float cz = XMVectorGetX(XMVector3Dot(centerPos, forward));

	// cascade별 snap 단위 (texel 배수) — cascade 2는 원거리라 거칠게 잡아 갱신 빈도↓
	static constexpr int SNAP_QUANT[CASCADE_COUNT] = { 1, 2, 4 };

	for (int i = 0; i < CASCADE_COUNT; ++i)
	{
		float cascadeSize = (&csmConstants.cascadeSplit.x)[i];
		float texelSize = (cascadeSize * 2.0f) / static_cast<float>(SHADOW_MAP_SIZE);
		float snapUnit = texelSize * SNAP_QUANT[i];

		// snap 경계 인덱스 — 정수 비교로 freeze 판단
		int snapXIdx = static_cast<int>(floorf(cx / snapUnit));
		int snapYIdx = static_cast<int>(floorf(cy / snapUnit));

		bool snapChanged = (snapXIdx != lastSnap[i].x) || (snapYIdx != lastSnap[i].y);
		cascadeDirty[i] = sunChanged || snapChanged;

		// freeze: snap도 sun도 그대로면 lightVP 그대로 둔다 (cz drift 무시 → cascadeBias로 흡수)
		if (!cascadeDirty[i])
			continue;

		float snapX = snapXIdx * snapUnit;
		float snapY = snapYIdx * snapUnit;

		// 스냅된 월드 타겟 복원
		XMVECTOR snappedTarget =
			XMVectorAdd(
				XMVectorAdd(XMVectorScale(right, snapX), XMVectorScale(trueUp, snapY)),
				XMVectorScale(forward, cz));
		XMVECTOR lightPos = XMVectorSubtract(snappedTarget, XMVectorScale(forward, shadowCasterDistance));

		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, snappedTarget, up);
		XMMATRIX lightProj = XMMatrixOrthographicLH(cascadeSize * 2.0f, cascadeSize * 2.0f, 0.1f, shadowCasterDistance * 2.0f);

		csmConstants.lightVP[i] = XMMatrixTranspose(XMMatrixMultiply(lightView, lightProj));

		lastSnap[i] = { snapXIdx, snapYIdx };
	}

	lastSunDir = currentSunDir;

	csmConstants.overheadMode = 0.0f;   // 캐스케이드 모드: 셰이더가 cascade 샘플
	csmConstantBuffer->CopyData(&csmConstants, sizeof(CascadeShadowConstants));
}

void ShadowMappingManager::UpdateOverheadShadow(const XMFLOAT3& center)
{
	// center(카메라 타겟) 바로 위에서 수직 아래로 내려보는 orthographic 프러스텀.
	// 동적 캐스터만 슬라이스 0에 그려지고, 셰이더는 lightVP[0]만 샘플한다.
	XMVECTOR c = XMLoadFloat3(&center);

	// 성당 중심축 바닥 조명 (FinalMapLightData.txt에서 추출한 x≈0 강한 floor light 스파인).
	// 그림자 방향만 여기서 끌어옴. 측면/벽/천장 조명은 제외 → 좌우로 튀는 현상 방지.
	// TODO: 추후 모델러가 별도 데이터로 제공 예정. 그때 이 고정 배열을 교체.
	static const XMFLOAT3 kCenterFloorLights[] = {
		{ -0.117f, 8.25f, -14.93f },
		{ -0.117f, 6.46f, -27.03f },
		{  0.102f, 8.85f, -35.08f },
		{  0.102f, 8.85f, -44.54f },
		{ -0.117f, 6.90f, -51.95f },
		{  0.015f, 8.24f, -72.63f },
		{ -0.247f, 8.89f, -84.60f },
		{  0.015f, 8.39f, -98.13f },
	};

	// 빛 진행 방향 = 하향(-Y) + 수평 lean. lean을 가장 가까운 중심 바닥 조명에서 끌어옴.
	XMVECTOR dir;
	if (overheadFollowNearestLight)
	{
		// 각 조명의 물리 lean(=수평거리/높이=tan 입사각)을 역제곱 거리 가중 평균으로 혼합.
		// 단일 nearest의 전환 스냅을 없애 방향이 연속적으로 회전 → 튐 제거.
		// 조명 바로 밑이면 lean≈0(짧음), 떨어질수록 lean↑(길어짐). overheadTilt = 최대 길이 클램프.
		XMFLOAT2 targetLean = { overheadLightDir.x, overheadLightDir.z };  // fallback
		float accX = 0.0f, accZ = 0.0f, wSum = 0.0f;
		for (int i = 0; i < (int)_countof(kCenterFloorLights); ++i)
		{
			float dx = center.x - kCenterFloorLights[i].x;
			float dz = center.z - kCenterFloorLights[i].z;
			float vy = kCenterFloorLights[i].y - center.y;   // 조명이 위 → 양수
			if (vy <= 1e-3f) continue;

			float d2 = dx * dx + dz * dz;
			float w = 1.0f / (d2 + 1.0f);   // 역제곱(eps=1: 조명 바로 위에서 발산 방지)
			accX += (dx / vy) * w;
			accZ += (dz / vy) * w;
			wSum += w;
		}
		if (wSum > 0.0f)
		{
			float lx = accX / wSum;
			float lz = accZ / wSum;
			float leanLen = sqrtf(lx * lx + lz * lz);
			if (leanLen > overheadTilt) { float s = overheadTilt / leanLen; lx *= s; lz *= s; }
			targetLean = { lx, lz };
		}

		// 조명 전환 시 방향 튐 방지 — 매 프레임 부드럽게 수렴
		const float smooth = 0.1f;
		overheadSmoothedLean.x += (targetLean.x - overheadSmoothedLean.x) * smooth;
		overheadSmoothedLean.y += (targetLean.y - overheadSmoothedLean.y) * smooth;

		dir = XMVector3Normalize(XMVectorSet(overheadSmoothedLean.x, -1.0f, overheadSmoothedLean.y, 0.0f));
	}
	else
	{
		dir = XMVector3Normalize(XMLoadFloat3(&overheadLightDir));  // 수동 고정 방향
	}

	XMVECTOR eye = XMVectorSubtract(c, XMVectorScale(dir, overheadHeight)); // 빛이 오는 위치(center 위쪽)

	// dir 기울기에 안정적인 up: 거의 수직이면 +Z, 많이 누우면 +Y 기준으로 직교 basis 구성
	float dirY = XMVectorGetY(dir);
	XMVECTOR upCand = (fabsf(dirY) > 0.9f) ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(upCand, dir));
	XMVECTOR up = XMVector3Cross(dir, right);

	XMMATRIX view = XMMatrixLookAtLH(eye, c, up);
	XMMATRIX proj = XMMatrixOrthographicLH(
		overheadHalfSize * 2.0f, overheadHalfSize * 2.0f, 0.1f, overheadHeight * 2.0f);

	csmConstants.lightVP[0] = XMMatrixTranspose(XMMatrixMultiply(view, proj));
	csmConstants.overheadMode = 1.0f;
	csmConstantBuffer->CopyData(&csmConstants, sizeof(CascadeShadowConstants));
}

void ShadowMappingManager::Invalidate()
{
	for (int i = 0; i < CASCADE_COUNT; ++i)
	{
		lastSnap[i] = { INT_MIN, INT_MIN };
		cascadeDirty[i] = true;
	}
	lastSunDir = { FLT_MAX, FLT_MAX, FLT_MAX };
}

void ShadowMappingManager::SettingsForCSM()
{
	XMVECTOR lightDir = XMVectorSet(-0.74f, -0.40f, -1.0f, 0);
	csmLightDir = XMVector3Normalize(lightDir);

	csmConstants.cascadeSplit = { 15.0f, 40.0f, 90.0f, 0.0f };
	csmConstants.shadowAmbientMin = 0.8f;
	csmConstants.shadowFloor = 0.0f;
	csmConstants.overheadMode = 0.0f;
	csmConstants.overheadStrength = 0.5f;
	csmConstants.overheadAmbientBoost = 1.5f;
	csmConstants.shadowPad2 = { 0.0f, 0.0f, 0.0f };
}

void ShadowMappingManager::CreateCSMResources(ID3D12Device* device)
{
	csmConstantBuffer = make_unique<UploadBuffer>();
	csmConstantBuffer->Initialize(device, sizeof(CascadeShadowConstants));

	D3D12_RESOURCE_DESC shadowDesc = {};
	shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	shadowDesc.Width = SHADOW_MAP_SIZE;
	shadowDesc.Height = SHADOW_MAP_SIZE;
	shadowDesc.DepthOrArraySize = CASCADE_COUNT;
	shadowDesc.MipLevels = 1;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &shadowDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		IID_PPV_ARGS(&csmTexture));
	MASSERT(SUCCEEDED(hr), "Failed to create shadowMap Texture!!\n");

	D3D12_DESCRIPTOR_HEAP_DESC shadowDSVDesc = {};
	shadowDSVDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	shadowDSVDesc.NumDescriptors = CASCADE_COUNT;
	shadowDSVDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&shadowDSVDesc, IID_PPV_ARGS(&csmDSVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create shadowMap DSV Heap!!\n");

	UINT dsvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = csmDSVHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < CASCADE_COUNT; ++i)
	{
		csmDSVHandle[i] = handle;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 1;
		device->CreateDepthStencilView(csmTexture.Get(), &dsvDesc, handle);

		handle.ptr += dsvSize;
	}

	OutputDebugStringA("Shadow Map creation succeed!!\n");
}

void ShadowMappingManager::CreateStaticCSMResources(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = SHADOW_MAP_SIZE;
	desc.Height = SHADOW_MAP_SIZE;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		IID_PPV_ARGS(&staticCsmTexture));
	MASSERT(SUCCEEDED(hr), "Failed to create static shadowMap Texture!!\n");

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&staticCsmDSVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create static shadowMap DSV Heap!!\n");

	staticCsmDSVHandle = staticCsmDSVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(staticCsmTexture.Get(), &dsvDesc, staticCsmDSVHandle);

	OutputDebugStringA("Static Shadow Map cache (cascade 2) created!!\n");
}

void ShadowMappingManager::CreateAtlasResources()
{
}
