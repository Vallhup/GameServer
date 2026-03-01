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
}
