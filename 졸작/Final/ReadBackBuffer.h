#pragma once

class ReadBackBuffer {
private:
    ComPtr<ID3D12Resource> mResource;
    UINT mSize = 0;

public:
    void Initialize(ID3D12Device* device, UINT sizeInBytes);
    void CopyFromGPU(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* srcResource);
    void ReadData(void* dest, size_t size);
    void Release();
};