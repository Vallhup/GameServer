#include "pch.h"
#include "SwapChain.h"

void SwapChain::Initialize(HWND hwnd, IDXGIFactory6* dxgi, ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
	CreateSwapChain(hwnd, dxgi, cmdQueue);
	CreateRenderTargetView(device);
}

void SwapChain::Present()
{
	swapchain->Present(0, 0);
}

void SwapChain::SwapIndex()
{
	backbufferindex = (backbufferindex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

ComPtr<ID3D12Resource> SwapChain::GetBackRTVBuffer() const
{
	return rtvbuffer[backbufferindex];
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetBackRTV() const
{
	return rtvhandle[backbufferindex];
}

void SwapChain::CreateSwapChain(HWND hwnd, IDXGIFactory6* dxgi, ID3D12CommandQueue* cmdQueue)
{
	swapchain.Reset();

	DXGI_SWAP_CHAIN_DESC sd = {
		.BufferDesc = {
			.Width = static_cast<UINT32>(WinSize.x),		
			.Height = static_cast<UINT32>(WinSize.y),		
			.RefreshRate = {
				.Numerator = 60,	
				.Denominator = 1	
				},
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,		
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED
			},
		.SampleDesc = {
			.Count = 1,		
			.Quality = 0
			},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,		 
		.BufferCount = SWAP_CHAIN_BUFFER_COUNT,
		.OutputWindow = hwnd,
		.Windowed = true,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,		
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	};

	dxgi->CreateSwapChain(cmdQueue, &sd, &swapchain);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		swapchain->GetBuffer(i, IID_PPV_ARGS(&rtvbuffer[i]));
}

void SwapChain::CreateRenderTargetView(ID3D12Device* device)
{
	int _rtvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvheap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvheap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		rtvhandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvHeapSize);
		device->CreateRenderTargetView(rtvbuffer[i].Get(), nullptr, rtvhandle[i]);
	}
}
