#include "pch.h"
#include "RenderTargets.h"
#include "ShadowMappingManager.h"
#include "Material.h"

void RenderTargets::Initialize(ID3D12Device* device, ShadowMappingManager* shadowMgr)
{
	CreateDepthStencilBuffer(device);
	CreateGBuffer(device);
	CreateFogRenderTarget(device);
	CreateDeferredRenderingDescriptors(device, shadowMgr);
	CreateHDRSceneRenderTarget(device);
}

void RenderTargets::AddSsaoSRV(ID3D12Device* device, ID3D12Resource* ssaoBlurRT)
{
	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = deferredSRVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize * 5;

	device->CreateShaderResourceView(ssaoBlurRT, nullptr, handle);
	OutputDebugStringA("SSAO SRV added\n");
}

void RenderTargets::CreateDepthStencilBuffer(ID3D12Device* device)
{
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, WinSize.x, WinSize.y);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimizedClearValue = CD3DX12_CLEAR_VALUE(dsvFormat, 1.0f, 0);

	device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&dsvBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dsvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(dsvBuffer.Get(), &dsvDesc, dsvHandle);
}

void RenderTargets::CreateGBuffer(ID3D12Device* device)
{
	OutputDebugStringA("Create G Buffer\n");

	D3D12_RESOURCE_DESC rtDesc = {};
	rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtDesc.Width = WinSize.x;
	rtDesc.Height = WinSize.y;
	rtDesc.DepthOrArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};

	// RT0: BaseColor.rgb + Metallic (RGBA8UN)
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = rtDesc.Format;
	memset(clearValue.Color, 0, sizeof(clearValue.Color));

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[0]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[0]");

	// RT1: Normal.xyz + Roughness (RGBA16F)
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[1]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[1]");

	// RT2: Emission.rgb + A0 (RGBA16F)
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[2]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[2]");

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 3;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&gBufferRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RTV Heap");

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gBufferRTVHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 3; ++i) {
		gBufferRTVHandles[i] = rtvHandle;
		device->CreateRenderTargetView(gBufferRT[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvSize;

		OutputDebugStringA(("G-Buffer RT" + to_string(i) + " RTV created\n").c_str());
	}

	OutputDebugStringA("G-Buffer created successfully!\n");
}

void RenderTargets::CreateFogRenderTarget(ID3D12Device* device)
{
	OutputDebugStringA("Create Fog Render Target\n");

	D3D12_RESOURCE_DESC rtDesc = {};
	rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtDesc.Width = WinSize.x;
	rtDesc.Height = WinSize.y;
	rtDesc.DepthOrArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue,
		IID_PPV_ARGS(&fogRT));
	MASSERT(SUCCEEDED(hr), "Failed to create Fog RT");

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&fogRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create Fog RTV Heap");

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = fogRTVHeap->GetCPUDescriptorHandleForHeapStart();

	fogRTVHandle = rtvHandle;
	device->CreateRenderTargetView(fogRT.Get(), nullptr, fogRTVHandle);

	OutputDebugStringA("Fog Render Target created successfully!\n");
}

void RenderTargets::CreateDeferredRenderingDescriptors(ID3D12Device* device, ShadowMappingManager* shadowMgr)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 8; // Gbuffer(3) + depth(1) + shadow(1) + Ssao(1) + FogRT(1) + PointShadow(1)
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&deferredSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create Deferred SRV Heap");

	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = deferredSRVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = deferredSRVHeap->GetGPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 3; ++i) {
		gBufferSRVHandles[i] = srvGpuHandle;
		device->CreateShaderResourceView(gBufferRT[i].Get(), nullptr, srvCpuHandle);

		srvCpuHandle.ptr += srvSize;
		srvGpuHandle.ptr += srvSize;

		OutputDebugStringA(("G-Buffer RT" + to_string(i) + " SRV created\n").c_str());
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
	depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrvDesc.Texture2D.MipLevels = 1;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(dsvBuffer.Get(), &depthSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
	shadowSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	shadowSrvDesc.Texture2DArray.MostDetailedMip = 0;
	shadowSrvDesc.Texture2DArray.MipLevels = 1;
	shadowSrvDesc.Texture2DArray.FirstArraySlice = 0;
	shadowSrvDesc.Texture2DArray.ArraySize = shadowMgr->GetCascadeCount();
	shadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(shadowMgr->GetCsmResource(), &shadowSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	OutputDebugStringA("Shadow Map SRV Created\n");

	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	device->CreateShaderResourceView(fogRT.Get(), nullptr, srvCpuHandle);
	OutputDebugStringA("Fog RT SRV Created\n");
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	// 슬롯 7: point shadow cube array (t14, root param 4)
	D3D12_SHADER_RESOURCE_VIEW_DESC pointShadowSrvDesc = {};
	pointShadowSrvDesc.Format = DXGI_FORMAT_R16_UNORM;
	pointShadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	pointShadowSrvDesc.TextureCubeArray.MostDetailedMip = 0;
	pointShadowSrvDesc.TextureCubeArray.MipLevels = 1;
	pointShadowSrvDesc.TextureCubeArray.First2DArrayFace = 0;
	pointShadowSrvDesc.TextureCubeArray.NumCubes = ShadowMappingManager::POINT_SHADOW_MAX_LIGHTS;
	pointShadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(shadowMgr->GetPointShadowResource(), &pointShadowSrvDesc, srvCpuHandle);
	pointShadowSRVHandle = srvGpuHandle;
	OutputDebugStringA("Point Shadow Cube Array SRV Created\n");

	OutputDebugStringA("Deferred Rendering Descriptors created successfully!!\n");
}

void RenderTargets::CreateHDRSceneRenderTarget(ID3D12Device* device)
{
	OutputDebugStringA("Create HDR Scene Render Target\n");

	D3D12_RESOURCE_DESC rtDesc = {};
	rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtDesc.Width = WinSize.x;
	rtDesc.Height = WinSize.y;
	rtDesc.DepthOrArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&HDRSceneRT));
	MASSERT(SUCCEEDED(hr), "Failed to create HDR Scene RT");

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&HDRSceneRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create HDR Scene RTV Heap");

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = HDRSceneRTVHeap->GetCPUDescriptorHandleForHeapStart();

	HDRSceneRTVHandle = rtvHandle;
	device->CreateRenderTargetView(HDRSceneRT.Get(), nullptr, HDRSceneRTVHandle);

	OutputDebugStringA("HDR Scene Render Target created successfully!\n");

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&HDRSceneSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create HDR Scene SRV Heap");

	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = HDRSceneSRVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = HDRSceneSRVHeap->GetGPUDescriptorHandleForHeapStart();

	HDRSceneSRVHandle = srvGpuHandle;
	device->CreateShaderResourceView(HDRSceneRT.Get(), nullptr, srvCpuHandle);

	OutputDebugStringA("HDR Scene RT SRV created\n");
}

void RenderTargets::RegisterHDRSceneToBindless(ID3D12Device* device)
{
	Material::RegisterHDRSceneSRV(device, HDRSceneRT.Get());
}
