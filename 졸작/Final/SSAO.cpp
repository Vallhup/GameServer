#include "pch.h"
#include "SSAO.h"

// ============================================================================
// Orthodox SSAO Implementation
// ============================================================================

void SSAO::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	OutputDebugStringA("=== Initializing Orthodox SSAO ===\n");

	CreateSSAOTexture(device);
	CreateViewSpaceGBuffer(device);
	CreateRandomTexture(device, cmdList);
	CreateDescriptorHeaps(device);
	CreateRTVs(device);
	CreateSRVs(device);
	BuildOffsetVectors();

	OutputDebugStringA("=== Orthodox SSAO Initialized Successfully ===\n");
}

// ============================================================================
// 1. SSAO 搬苞 咆胶贸 积己
// ============================================================================

void SSAO::CreateSSAOTexture(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = WinSize.x;
	desc.Height = WinSize.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8_UNORM;  // SSAO绰 grayscale
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = desc.Format;
	clearValue.Color[0] = 1.0f;
	clearValue.Color[1] = 1.0f;
	clearValue.Color[2] = 1.0f;
	clearValue.Color[3] = 1.0f;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&ssaoTexture));

	MASSERT(SUCCEEDED(hr), "Failed to create SSAO Texture");

	OutputDebugStringA("SSAO Texture created\n");
}

// ============================================================================
// 2. View Space G-Buffer 积己 (Normal + Position)
// ============================================================================

void SSAO::CreateViewSpaceGBuffer(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = WinSize.x;
	desc.Height = WinSize.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;  // 4 channel float
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = desc.Format;
	memset(clearValue.Color, 0, sizeof(clearValue.Color));

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	// View Normal
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&viewNormal));

	MASSERT(SUCCEEDED(hr), "Failed to create View Normal");

	// View Position
	hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&viewPosition));

	MASSERT(SUCCEEDED(hr), "Failed to create View Position");

	OutputDebugStringA("View Space G-Buffer created\n");
}

// ============================================================================
// 3. Random Vector Texture 积己 (256x256)
// ============================================================================

void SSAO::CreateRandomTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	// === Texture Description ===
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// === Default Heap (GPU) ===
	CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&randomTexture));

	MASSERT(SUCCEEDED(hr), "Failed to create Random Texture");

	// === Upload Buffer ===
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(randomTexture.Get(), 0, 1);

	CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	hr = device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&randomTextureUploadBuffer));

	MASSERT(SUCCEEDED(hr), "Failed to create Random Texture Upload Buffer");

	// === Random Data 积己 ===
	std::vector<UINT32> randomData(256 * 256);

	srand(static_cast<unsigned int>(time(nullptr)));

	for (int i = 0; i < 256 * 256; ++i)
	{
		// Random 3D vector [0, 1] 裹困
		float x = static_cast<float>(rand()) / RAND_MAX;
		float y = static_cast<float>(rand()) / RAND_MAX;
		float z = static_cast<float>(rand()) / RAND_MAX;

		// [0, 1] ℃ [0, 255]
		UINT32 r = static_cast<UINT32>(x * 255.0f);
		UINT32 g = static_cast<UINT32>(y * 255.0f);
		UINT32 b = static_cast<UINT32>(z * 255.0f);
		UINT32 a = 255;

		randomData[i] = (a << 24) | (b << 16) | (g << 8) | r;  // RGBA
	}

	// === Upload to GPU ===
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = randomData.data();
	subResourceData.RowPitch = 256 * sizeof(UINT32);
	subResourceData.SlicePitch = subResourceData.RowPitch * 256;

	UpdateSubresources(cmdList, randomTexture.Get(), randomTextureUploadBuffer.Get(),
		0, 0, 1, &subResourceData);

	// Transition to SRV
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		randomTexture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	cmdList->ResourceBarrier(1, &barrier);

	OutputDebugStringA("Random Texture created\n");
}

// ============================================================================
// 4. Descriptor Heaps 积己
// ============================================================================

void SSAO::CreateDescriptorHeaps(ID3D12Device* device)
{
	// === RTV Heap (3俺: SSAO + ViewNormal + ViewPosition) ===
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 3;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create SSAO RTV Heap");

	// === SRV Heap (4俺: SSAO + ViewNormal + ViewPosition + Random) ===
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create SSAO SRV Heap");

	OutputDebugStringA("SSAO Descriptor Heaps created\n");
}

// ============================================================================
// 5. RTVs 积己
// ============================================================================

void SSAO::CreateRTVs(ID3D12Device* device)
{
	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// SSAO RTV
	ssaoRTV = rtvHandle;
	device->CreateRenderTargetView(ssaoTexture.Get(), nullptr, ssaoRTV);
	rtvHandle.ptr += rtvSize;

	// View Normal RTV
	viewNormalRTV = rtvHandle;
	device->CreateRenderTargetView(viewNormal.Get(), nullptr, viewNormalRTV);
	rtvHandle.ptr += rtvSize;

	// View Position RTV
	viewPositionRTV = rtvHandle;
	device->CreateRenderTargetView(viewPosition.Get(), nullptr, viewPositionRTV);

	OutputDebugStringA("SSAO RTVs created\n");
}

// ============================================================================
// 6. SRVs 积己
// ============================================================================

void SSAO::CreateSRVs(ID3D12Device* device)
{
	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

	// SSAO SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC ssaoSrvDesc = {};
	ssaoSrvDesc.Format = DXGI_FORMAT_R8_UNORM;
	ssaoSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	ssaoSrvDesc.Texture2D.MipLevels = 1;
	ssaoSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	ssaoSRV = srvGpuHandle;
	device->CreateShaderResourceView(ssaoTexture.Get(), &ssaoSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	// View Normal SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC viewSrvDesc = {};
	viewSrvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	viewSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	viewSrvDesc.Texture2D.MipLevels = 1;
	viewSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	viewNormalSRV = srvGpuHandle;
	device->CreateShaderResourceView(viewNormal.Get(), &viewSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	// View Position SRV
	viewPositionSRV = srvGpuHandle;
	device->CreateShaderResourceView(viewPosition.Get(), &viewSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	// Random Texture SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC randomSrvDesc = {};
	randomSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	randomSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	randomSrvDesc.Texture2D.MipLevels = 1;
	randomSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	randomSRV = srvGpuHandle;
	device->CreateShaderResourceView(randomTexture.Get(), &randomSrvDesc, srvCpuHandle);

	OutputDebugStringA("SSAO SRVs created\n");
}

// ============================================================================
// 7. Offset Vectors 积己 (Frank Luna 规侥)
// ============================================================================

void SSAO::BuildOffsetVectors()
{
	// 8 cube corners
	offsetVectors[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	offsetVectors[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	offsetVectors[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	offsetVectors[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	offsetVectors[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	offsetVectors[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	offsetVectors[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	offsetVectors[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	offsetVectors[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	offsetVectors[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	offsetVectors[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	offsetVectors[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	offsetVectors[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	offsetVectors[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	// Random length [0.25, 1.0]
	for (int i = 0; i < 14; ++i)
	{
		float s = 0.25f + (static_cast<float>(rand()) / RAND_MAX) * 0.75f;

		XMVECTOR v = XMLoadFloat4(&offsetVectors[i]);
		v = XMVector4Normalize(v);
		v = XMVectorScale(v, s);

		XMStoreFloat4(&offsetVectors[i], v);
	}

	OutputDebugStringA("Offset Vectors built\n");
}

// ============================================================================
// Getters
// ============================================================================

void SSAO::GetOffsetVectors(XMFLOAT4 outOffsets[14]) const
{
	for (int i = 0; i < 14; ++i)
	{
		outOffsets[i] = offsetVectors[i];
	}
}