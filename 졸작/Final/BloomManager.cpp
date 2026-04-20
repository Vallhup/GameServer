#include "pch.h"
#include "BloomManager.h"

void BloomManager::Initialize(ID3D12Device* device)
{
	CreateBloomResource(device);
	CreateMipRTVs(device);
}

void BloomManager::CreateBloomResource(ID3D12Device* device)
{
	UINT baseWidth = max(1u, (UINT)(WinSize.x / 2));
	UINT baseHeight = max(1u, (UINT)(WinSize.y / 2));

	UINT w = baseWidth;
	UINT h = baseHeight;
	for (UINT i = 0; i < CHAIN_LENGTH; ++i)
	{
		mipWidths[i] = w;
		mipHeights[i] = h;
		w = max(1u, w / 2);
		h = max(1u, h / 2);
	}

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = baseWidth;
	desc.Height = baseHeight;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = CHAIN_LENGTH;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue,
		IID_PPV_ARGS(&bloomTex));
	MASSERT(SUCCEEDED(hr), "Failed to create Bloom mip-chain texture");

	OutputDebugStringA("Bloom mip-chain texture created\n");
}

void BloomManager::CreateMipRTVs(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = CHAIN_LENGTH;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create Bloom RTV heap");

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < CHAIN_LENGTH; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = i;
		rtvDesc.Texture2D.PlaneSlice = 0;

		device->CreateRenderTargetView(bloomTex.Get(), &rtvDesc, rtvHandle);
		mipRTVHandles[i] = rtvHandle;
		rtvHandle.ptr += rtvSize;
	}

	OutputDebugStringA("Bloom mip RTVs created\n");
}

void BloomManager::RegisterMipsToBindless(ID3D12Device* device)
{
	for (UINT i = 0; i < CHAIN_LENGTH; ++i)
	{
		Material::RegisterBloomMipSRV(device, bloomTex.Get(), i, i);
	}
}
