#include "pch.h"
#include "SwapChain.h"
#include "Timer.h"

void SwapChain::Initialize(IDXGIFactory7* dxgi, ID3D12CommandQueue* cmdQueue,
    ID3D12Device* device, HWND hwnd)
{
    CreateSwapChain(dxgi, cmdQueue, hwnd);
    CreateRenderTargetView(device);
}

void SwapChain::CreateSwapChain(IDXGIFactory7* dxgi, ID3D12CommandQueue* cmdQueue, HWND hwnd)
{
	ComPtr<IDXGISwapChain> tempSwapChain;

	ComPtr<IDXGIAdapter> adapter;
	HRESULT hr = dxgi->EnumAdapters(0, &adapter);
	MASSERT(SUCCEEDED(hr), "Failed to get adapter!");

	ComPtr<IDXGIOutput> output;
	hr = adapter->EnumOutputs(0, &output);
	MASSERT(SUCCEEDED(hr), "Failed to get output!");

	UINT numModes = 0;
	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr);

	vector<DXGI_MODE_DESC> displayModes(numModes);
	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModes.data());

	DXGI_MODE_DESC bestMode = {};
	float bestRefreshRate = 0.0f;

	OutputDebugStringA("=== Matching Modes ===\n");

	for (const auto& mode : displayModes) {
		if (mode.Width == WinSize.x && mode.Height == WinSize.y) {
			float refreshRate = static_cast<float>(mode.RefreshRate.Numerator) / mode.RefreshRate.Denominator;

			string matchInfo = "Match found: " + to_string(mode.Width) + "x" + to_string(mode.Height) +
				" @ " + to_string(refreshRate) + "Hz (" +
				to_string(mode.RefreshRate.Numerator) + "/" +
				to_string(mode.RefreshRate.Denominator) + ")\n";
			OutputDebugStringA(matchInfo.c_str());

			if (refreshRate > bestRefreshRate) {
				bestMode = mode;
				bestRefreshRate = refreshRate;
				OutputDebugStringA("  -> New best mode selected!\n");
			}
			else {
				OutputDebugStringA("  -> Lower refresh rate, skipped\n");
			}
		}
	}

	if (bestRefreshRate == 0.0f) {
		OutputDebugStringA("No matching mode found! Using default 60Hz\n");
		bestMode.RefreshRate.Numerator = 60;
		bestMode.RefreshRate.Denominator = 1;
		bestMode.Width = WinSize.x;
		bestMode.Height = WinSize.y;
		bestRefreshRate = 60.0f;
	}

	OutputDebugStringA("=== FINAL SELECTION ===\n");
	OutputDebugStringA(("Selected refresh rate: " + to_string(bestRefreshRate) + "Hz (" +
		to_string(bestMode.RefreshRate.Numerator) + "/" +
		to_string(bestMode.RefreshRate.Denominator) + ")\n").c_str());

	TIMER.SetTargetFPS(bestRefreshRate);
	nativeRefresh = static_cast<int>(bestRefreshRate + 0.5f);

	DXGI_SWAP_CHAIN_DESC sd = {
		.BufferDesc = {
			.Width = static_cast<UINT32>(WinSize.x),
			.Height = static_cast<UINT32>(WinSize.y),
			.RefreshRate = bestMode.RefreshRate,
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

	hr = dxgi->CreateSwapChain(cmdQueue, &sd, &tempSwapChain);
	MASSERT(SUCCEEDED(hr), "Failed to create SwapChain");

	hr = tempSwapChain.As(&swapChain);
	MASSERT(SUCCEEDED(hr), "Failed to cast to IDXGISwapChain4");

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));

	backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	OutputDebugStringA("=== SwapChain Creation Complete ===\n");
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

	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvViewDesc = {};
	rtvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvHeapSize);
		device->CreateRenderTargetView(rtvBuffer[i].Get(), &rtvViewDesc, rtvHandle[i]);
	}
}

HRESULT SwapChain::Present()
{
	HRESULT hr = swapChain->Present(0, 0);
    backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	return hr;
}

void SwapChain::ResizeBuffers(ID3D12Device* device)
{
    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        rtvBuffer[i].Reset();

    swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));

    CreateRenderTargetView(device);
    backBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

IDXGISwapChain4* SwapChain::GetSwapChain() const 
{ 
    return swapChain.Get(); 
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentRTV() const
{ 
    return rtvHandle[backBufferIndex]; 
}

ID3D12Resource* SwapChain::GetCurrentBuffer() const 
{
    return rtvBuffer[backBufferIndex].Get(); 
}

UINT32 SwapChain::GetBackBufferIndex() const 
{
    return backBufferIndex; 
}