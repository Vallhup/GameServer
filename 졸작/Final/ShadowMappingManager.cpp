#include "pch.h"
#include "ShadowMappingManager.h"

void ShadowMappingManager::Initialize(ID3D12Device* device)
{
	SettingsForCSM();
	CreateCSMResources(device);
	CreateAtlasResources();
}

void ShadowMappingManager::UpdateCascadeShadow(const XMFLOAT3& center)
{
	XMVECTOR centerPos = XMLoadFloat3(&center);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	float shadowCasterDistance = 80.0f;

	XMVECTOR lightPos = XMVectorSubtract(centerPos, XMVectorScale(csmLightDir, shadowCasterDistance));
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, centerPos, up);

	for (int i = 0; i < CASCADE_COUNT; ++i)
	{
		float cascadeSize = (&csmConstants.cascadeSplit.x)[i];

		// 광원 투영 = cascadeSize 기반
		XMMATRIX lightProj = XMMatrixOrthographicLH(cascadeSize * 2.0f, cascadeSize * 2.0f, 0.1f, shadowCasterDistance * 2.0f);

		csmConstants.lightVP[i] = XMMatrixTranspose(XMMatrixMultiply(lightView, lightProj));
	}

	csmConstantBuffer->CopyData(&csmConstants, sizeof(CascadeShadowConstants));
}

void ShadowMappingManager::SettingsForCSM()
{
	XMVECTOR lightDir = XMVectorSet(-45.0f, -40.0f, -60.0f, 0);
	csmLightDir = XMVector3Normalize(lightDir);

	csmConstants.cascadeSplit = { 10.0f, 30.0f, 80.0f, 0.0f };
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

void ShadowMappingManager::CreateAtlasResources()
{
}
