#include "pch.h"
#include "SSAO.h"
#include "RenderTargets.h"

void SSAO::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderTargets* rt)
{
	CreateSSAOResources(device, cmdList, rt);
}

void SSAO::CreateSSAOResources(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderTargets* rt)
{
	D3D12_RESOURCE_DESC ssaoDesc = {};
	ssaoDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ssaoDesc.Width = WinSize.x / 2;
	ssaoDesc.Height = WinSize.y / 2;
	ssaoDesc.DepthOrArraySize = 1;
	ssaoDesc.MipLevels = 1;
	ssaoDesc.Format = DXGI_FORMAT_R8_UNORM;
	ssaoDesc.SampleDesc.Count = 1;
	ssaoDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8_UNORM;
	clearValue.Color[0] = 1.0f;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ssaoDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&ssaoRT));
	MASSERT(SUCCEEDED(hr), "Failed to create ssao RT");

	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ssaoDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&ssaoBlurRT));
	MASSERT(SUCCEEDED(hr), "Failed to create ssao blur RT");

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&ssaoRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create ssao RTV Heap");

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	ssaoRTVHandle = ssaoRTVHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateRenderTargetView(ssaoRT.Get(), nullptr, ssaoRTVHandle);

	OutputDebugStringA("Ssao RTV Created!!\n");

	ssaoBlurRTVHandle = ssaoRTVHandle;
	ssaoBlurRTVHandle.ptr += rtvSize;
	device->CreateRenderTargetView(ssaoBlurRT.Get(), nullptr, ssaoBlurRTVHandle);

	OutputDebugStringA("Ssao Blur RTV Created!!\n");

	GenerateSampleKernel(device);
	GenerateNoiseTexture(device, cmdList);
	CreateSRVs(device, rt);
}

void SSAO::GenerateSampleKernel(ID3D12Device* device)
{
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<float> dist(0.0f, 1.0f);
	
	for (int i = 0; i < 32; ++i)
	{
		XMFLOAT3 sample = { dist(gen) * 2.0f - 1.0f, dist(gen) * 2.0f - 1.0f , dist(gen) };

		XMVECTOR v = XMLoadFloat3(&sample);
		v = XMVector3Normalize(v);

		float scale = (float)i / 32.0f;
		scale = 0.1f + scale * scale * 0.9f;  // 중심에 더 밀집
		v = XMVectorScale(v, scale);

		XMStoreFloat3((XMFLOAT3*)&ssaoConstant.samples[i], v);
		ssaoConstant.samples[i].w = 0.0f;
	}

	ssaoConstant.noiseScale = XMFLOAT2(WinSize.x / 2.0f / 4.0f, WinSize.y / 2.0f / 4.0f);
	ssaoConstant.samplingRadius = 0.5f;
	ssaoConstant.ssaoBias = 0.025f;

	ssaoCB = make_unique<UploadBuffer>();
	ssaoCB->Initialize(device, sizeof(SSAOConstants));
	ssaoCB->CopyData(&ssaoConstant, sizeof(SSAOConstants));
}

void SSAO::GenerateNoiseTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<float> dist(-1.0f, 1.0f);

	struct NoisePixel { float x, y, z, w; };
	NoisePixel noiseData[16];

	for (int i = 0; i < 16; i++)
	{
		noiseData[i] = { dist(gen), dist(gen), 0.0f, 0.0f };
	}

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = 4;
	texDesc.Height = 4;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
		&texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&noiseTexture));
	MASSERT(SUCCEEDED(hr), "Failed to ssao noise texture");

	UINT64 uploadSize = GetRequiredIntermediateSize(noiseTexture.Get(), 0, 1);
	CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&noiseUploadBuffer));
	MASSERT(SUCCEEDED(hr), "Failed to ssao noise upload buffer");

	D3D12_SUBRESOURCE_DATA subresource = {};
	subresource.pData = noiseData;
	subresource.RowPitch = 4 * sizeof(NoisePixel);
	subresource.SlicePitch = subresource.RowPitch * 4;

	UpdateSubresources(cmdList, noiseTexture.Get(), noiseUploadBuffer.Get(), 0, 0, 1, &subresource);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		noiseTexture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);
}

void SSAO::CreateSRVs(ID3D12Device* device, RenderTargets* rt)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeap = {};
	srvHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeap.NumDescriptors = 4;
	srvHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = device->CreateDescriptorHeap(&srvHeap, IID_PPV_ARGS(&ssaoSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create ssao Heap");

	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = ssaoSRVHeap->GetCPUDescriptorHandleForHeapStart();

	device->CreateShaderResourceView(rt->GetGBuffer(1), nullptr, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
	depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrvDesc.Texture2D.MipLevels = 1;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(rt->GetDepthBuffer(), &depthSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;

	device->CreateShaderResourceView(noiseTexture.Get(), nullptr, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;

	device->CreateShaderResourceView(ssaoRT.Get(), nullptr, srvCpuHandle);
}
