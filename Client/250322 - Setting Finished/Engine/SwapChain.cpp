#include "pch.h"
#include "SwapChain.h"

void SwapChain::Init(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
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
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
}

void SwapChain::Present()
{
	_swapChain->Present(0, 0);	// 첫 인자 - 수직 동기화 끄기(0), 켜기(1)
}

void SwapChain::SwapIndex()
{
	_backbufferIndex = (_backbufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}
