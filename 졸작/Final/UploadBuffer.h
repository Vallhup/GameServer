#pragma once

class UploadBuffer
{
public:
    void Initialize(ID3D12Device* device, size_t sizeInBytes);

    void CopyData(const void* src, size_t size, size_t offset = 0);

    ID3D12Resource* GetResource() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

private:
    ComPtr<ID3D12Resource> mResource;
    UINT8* mMappedData = nullptr;
    size_t mSize = 0;
};
