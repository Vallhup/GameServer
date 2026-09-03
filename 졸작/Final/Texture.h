#pragma once

class Texture
{
public:
    void InitializeDDS(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath);
    void InitializeCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& ddsPath);
    void InitializeLUT(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath);
    void InitializeFromMemory(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, UINT width, UINT height, DXGI_FORMAT format);

    ID3D12Resource* GetTexture() const { return texture.Get(); }
    void ReleaseUploadBuffer() { uploadBuffer.Reset(); }

private:
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> uploadBuffer;
};