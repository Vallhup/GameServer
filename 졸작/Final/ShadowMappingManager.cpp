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

	const float shadowCasterDistance = 120.0f;

	// centerPos에 의존하지 않는 라이트 공간 축 (shadow swimming 방지용)
	XMVECTOR forward = XMVector3Normalize(csmLightDir);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
	XMVECTOR trueUp = XMVector3Normalize(XMVector3Cross(forward, right));

	float cx = XMVectorGetX(XMVector3Dot(centerPos, right));
	float cy = XMVectorGetX(XMVector3Dot(centerPos, trueUp));
	float cz = XMVectorGetX(XMVector3Dot(centerPos, forward));

	for (int i = 0; i < CASCADE_COUNT; ++i)
	{
		float cascadeSize = (&csmConstants.cascadeSplit.x)[i];
		float texelSize = (cascadeSize * 2.0f) / static_cast<float>(SHADOW_MAP_SIZE);

		// texel 경계 인덱스 — 정수 비교로 freeze 판단
		int snapXIdx = static_cast<int>(floorf(cx / texelSize));
		int snapYIdx = static_cast<int>(floorf(cy / texelSize));

		bool snapChanged = (snapXIdx != lastSnap[i].x) || (snapYIdx != lastSnap[i].y);
		cascadeDirty[i] = sunChanged || snapChanged;

		// freeze: snap도 sun도 그대로면 lightVP 그대로 둔다 (cz drift 무시 → cascadeBias로 흡수)
		if (!cascadeDirty[i])
			continue;

		float snapX = snapXIdx * texelSize;
		float snapY = snapYIdx * texelSize;

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

	csmConstants.cascadeSplit = { 15.0f, 40.0f, 100.0f, 0.0f };
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
