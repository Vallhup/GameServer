#include "pch.h"
#include "SwapChain.h"
#include "Engine.h"

void SwapChain::Init(const WindowInfo& info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	CreateSwapChain(info, dxgi, cmdQueue);
	CreateRenderTargetView(device);
}

void SwapChain::Present()
{
	_swapChain->Present(0, 0);	// 첫 인자 - 수직 동기화 끄기(0), 켜기(1)
}

void SwapChain::SwapIndex()
{
	_backbufferIndex = (_backbufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void SwapChain::CreateSwapChain(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapDesc =
	{
		.BufferDesc =
		{
			.Width = static_cast<uint32>(info.width),
			.Height = static_cast<uint32>(info.height),
			.RefreshRate =
			{
				.Numerator = 60,
				.Denominator = 1
			},
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED
		},
		.SampleDesc =
		{
			.Count = 1,
			.Quality = 0
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = SWAP_CHAIN_BUFFER_COUNT,
		.OutputWindow = info.hwnd,
		.Windowed = info.windowed,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,	// 전&후면 버퍼 교체 시 이전 프레임 정보 버림
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	};

	dxgi->CreateSwapChain(cmdQueue.Get(), &swapDesc, &_swapChain);

	for (int32 i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_rtvBuffer[i]));
}

void SwapChain::CreateRenderTargetView(ComPtr<ID3D12Device> device)
{
	int32 rtvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc =
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	DEVICE->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&_rtvHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		_rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * rtvHeapSize);
		device->CreateRenderTargetView(_rtvBuffer[i].Get(), nullptr, _rtvHandle[i]);
	}
}