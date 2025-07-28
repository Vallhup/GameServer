#pragma once

class SwapChain
{
public:
	void Initialize(HWND hwnd, IDXGIFactory6* dxgi, ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
	void Present();
	void SwapIndex();

	ComPtr<ID3D12Resource> GetBackRTVBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackRTV() const;

private:
	void CreateSwapChain(HWND hwnd, IDXGIFactory6* dxgi, ID3D12CommandQueue* cmdQueue);
	void CreateRenderTargetView(ID3D12Device* device);

private:
	ComPtr<IDXGISwapChain> swapchain;
	
	ComPtr<ID3D12Resource>			rtvbuffer[SWAP_CHAIN_BUFFER_COUNT];
	ComPtr<ID3D12DescriptorHeap>	rtvheap;
	D3D12_CPU_DESCRIPTOR_HANDLE		rtvhandle[SWAP_CHAIN_BUFFER_COUNT];

	UINT32 backbufferindex = 0;
};

