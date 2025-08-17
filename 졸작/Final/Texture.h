#pragma once

class Texture
{
public:
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath);
    void InitializeFromRAW(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath, UINT width, UINT height);

    ID3D12Resource* GetTexture() const { return texture.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return srvGpuHandle; }
    void ReleaseUploadBuffer() { uploadBuffer.Reset(); }

private:
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
};