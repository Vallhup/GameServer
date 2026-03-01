#include "pch.h"
#include "SSAO.h"

void SSAO::Initialize(ID3D12Device* device)
{
	CreateSSAOResources(device);
}

void SSAO::CreateSSAOResources(ID3D12Device* device)
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
		D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&ssaoBlurRT));
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

	GenerateSampleKernel();

	ssaoCB = make_unique<UploadBuffer>();
	ssaoCB->Initialize(device, sizeof(SSAOConstants));
	ssaoCB->CopyData(&ssaoConstant, sizeof(SSAOConstants));
}

void SSAO::GenerateSampleKernel()
{
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<float> dist(0.0f, 1.0f);
	
	for (int i = 0; i < 16; ++i)
	{
		XMFLOAT3 sample = { dist(gen) * 2.0f - 1.0f, dist(gen) * 2.0f - 1.0f , dist(gen) };

		XMVECTOR v = XMLoadFloat3(&sample);
		v = XMVector3Normalize(v);

		float scale = (float)i / 16.0f;
		scale = 0.1f + scale * scale * 0.9f;  // 중심에 더 밀집
		v = XMVectorScale(v, scale);

		XMStoreFloat3((XMFLOAT3*)&ssaoConstant.samples[i], v);
		ssaoConstant.samples[i].w = 0.0f;
	}

	ssaoConstant.noiseScale = XMFLOAT2(WinSize.x / 2.0f / 4.0f, WinSize.y / 2.0f / 4.0f);
	ssaoConstant.samplingRadius = 0.5f;
	ssaoConstant.padding = 0.0f;
}
