#pragma once

class DX12Core;

class HiZCuller
{
public:
    void Initialize(ID3D12Device* device, UINT w, UINT h, ID3D12Resource* depthBuffer);
    void GenerateHiZ(DX12Core& core);

    ID3D12Resource* GetHiZTexture() const { return hiZTexture.Get(); }
    UINT GetMipLevels() const { return mipLevels; }

private:
    ComPtr<ID3D12Resource> hiZTexture;
    ComPtr<ID3D12DescriptorHeap> hiZHeap;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> generatePSO;
    UINT width = 0;
    UINT height = 0;
    UINT mipLevels = 0;
    UINT descSize = 0;
};
