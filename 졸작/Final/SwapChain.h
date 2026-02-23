#pragma once

class SwapChain
{
public:
    void Initialize(IDXGIFactory7* dxgi, ID3D12CommandQueue* cmdQueue, ID3D12Device* device, HWND hwnd);

    HRESULT Present();
    void ResizeBuffers(ID3D12Device* device);

    IDXGISwapChain4* GetSwapChain() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
    ID3D12Resource* GetCurrentBuffer() const;
    UINT32 GetBackBufferIndex() const;

private:
    void CreateSwapChain(IDXGIFactory7* dxgi, ID3D12CommandQueue* cmdQueue, HWND hwnd);
    void CreateRenderTargetView(ID3D12Device* device);

private:
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12Resource> rtvBuffer[SWAP_CHAIN_BUFFER_COUNT];
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[SWAP_CHAIN_BUFFER_COUNT];
    UINT32 backBufferIndex = 0;
};

