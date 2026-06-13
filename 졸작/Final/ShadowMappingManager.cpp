#include "pch.h"
#include "ShadowMappingManager.h"
#include "LightManager.h"

void ShadowMappingManager::Initialize(ID3D12Device* device)
{
	Invalidate();
	SettingsForCSM();
	CreateCSMResources(device);
	CreateStaticCSMResources(device);
	CreatePointShadowResources(device);
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

	XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&overheadLightDir));  // 고정 방향
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
	csmConstants.overheadStrength = 0.25f;
	csmConstants.overheadAmbientBoost = 1.5f;
	csmConstants.pointShadowCount = 0.0f;
	csmConstants.pointShadowStrength = 1.0f;
	csmConstants.pointShadowNear = POINT_SHADOW_NEAR;
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

void ShadowMappingManager::CreatePointShadowResources(ID3D12Device* device)
{
	const UINT sliceCount = POINT_SHADOW_MAX_LIGHTS * 6;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = POINT_SHADOW_MAP_SIZE;
	desc.Height = POINT_SHADOW_MAP_SIZE;
	desc.DepthOrArraySize = sliceCount;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R16_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D16_UNORM;
	clearValue.DepthStencil.Depth = 1.0f;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		IID_PPV_ARGS(&pointShadowTexture));
	MASSERT(SUCCEEDED(hr), "Failed to create point shadow cube array!!\n");

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = sliceCount;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&pointShadowDSVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create point shadow DSV Heap!!\n");

	pointShadowDsvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	UINT dsvSize = pointShadowDsvSize;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = pointShadowDSVHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < sliceCount; ++i)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D16_UNORM;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 1;
		device->CreateDepthStencilView(pointShadowTexture.Get(), &dsvDesc, handle);

		handle.ptr += dsvSize;
	}

	pointShadowFaceCBPool = make_unique<UploadBuffer>();
	pointShadowFaceCBPool->Initialize(device, CONSTANT_BUFFER_ALIGNMENT * sliceCount);

	pointShadowInstancePool = make_unique<UploadBuffer>();
	pointShadowInstancePool->Initialize(device, sizeof(XMMATRIX) * 65536);  // 4MB

	OutputDebugStringA("Point shadow cube array created!!\n");
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowMappingManager::GetPointShadowDSV(int slice) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = pointShadowDSVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(pointShadowDsvSize) * slice;
	return handle;
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowMappingManager::WritePointShadowFaceCB(int slice, const XMFLOAT3& lightPos, int face, float range)
{
	static const XMVECTORF32 kFaceDirs[6] = {
		{ {  1,  0,  0, 0 } }, { { -1,  0,  0, 0 } },
		{ {  0,  1,  0, 0 } }, { {  0, -1,  0, 0 } },
		{ {  0,  0,  1, 0 } }, { {  0,  0, -1, 0 } },
	};
	static const XMVECTORF32 kFaceUps[6] = {
		{ { 0, 1,  0, 0 } }, { { 0, 1,  0, 0 } },
		{ { 0, 0, -1, 0 } }, { { 0, 0,  1, 0 } },
		{ { 0, 1,  0, 0 } }, { { 0, 1,  0, 0 } },
	};

	XMVECTOR eye = XMLoadFloat3(&lightPos);
	XMMATRIX view = XMMatrixLookToLH(eye, kFaceDirs[face], kFaceUps[face]);

	float nearZ = csmConstants.pointShadowNear;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, nearZ, max(range, nearZ + 0.01f));

	CascadeShadowConstants faceConstants = {};
	faceConstants.lightVP[0] = XMMatrixTranspose(XMMatrixMultiply(view, proj));

	size_t offset = static_cast<size_t>(slice) * CONSTANT_BUFFER_ALIGNMENT;
	pointShadowFaceCBPool->CopyData(&faceConstants, sizeof(CascadeShadowConstants), offset);
	return pointShadowFaceCBPool->GetGPUVirtualAddress() + offset;
}

void ShadowMappingManager::TransitionPointShadowToDepthWrite(ID3D12GraphicsCommandList* cmd)
{
	if (!pointShadowInSrvState)
		return;

	D3D12_RESOURCE_BARRIER toDw = CD3DX12_RESOURCE_BARRIER::Transition(
		pointShadowTexture.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	cmd->ResourceBarrier(1, &toDw);
	pointShadowInSrvState = false;
}

void ShadowMappingManager::TransitionPointShadowToShaderResource(ID3D12GraphicsCommandList* cmd)
{
	if (pointShadowInSrvState)
		return;

	D3D12_RESOURCE_BARRIER toSrv = CD3DX12_RESOURCE_BARRIER::Transition(
		pointShadowTexture.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmd->ResourceBarrier(1, &toSrv);
	pointShadowInSrvState = true;
}
