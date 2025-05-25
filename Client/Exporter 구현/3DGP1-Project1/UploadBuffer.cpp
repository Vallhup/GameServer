#include "pch.h"
#include "UploadBuffer.h"

void UploadBuffer::Initialize(ID3D12Device* device, UINT sizeInBytes)
{
    // 256바이트 정렬 필수
    mSize = (sizeInBytes + 255) & ~255;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mResource)
    );
    MASSERT(SUCCEEDED(hr), "Failed to create ConstantBuffer resource");

    hr = mResource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
    MASSERT(SUCCEEDED(hr), "Failed to map ConstantBuffer");
}

void UploadBuffer::CopyData(const void* src, size_t size, size_t offset)
{
    memcpy(mMappedData + offset, src, size);
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::GetGPUVirtualAddress() const
{
     return mResource->GetGPUVirtualAddress();
}
