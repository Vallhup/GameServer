#pragma once

enum class BufferType {
    Upload,
    UAV
};

template <BufferType T>
class GPUBuffer
{
public:
    void Initialize(ID3D12Device* device, size_t sizeInBytes) {
        mSize = (sizeInBytes + 255) & ~255;

        constexpr D3D12_HEAP_TYPE heapType = (T == BufferType::Upload) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
        constexpr D3D12_RESOURCE_FLAGS flags = (T == BufferType::Upload) ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        constexpr D3D12_RESOURCE_STATES states = (T == BufferType::Upload) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        CD3DX12_HEAP_PROPERTIES heapProps(heapType);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mSize, flags);

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            states,
            nullptr,
            IID_PPV_ARGS(&mResource)
        );

        if constexpr (T == BufferType::Upload)
            mResource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
    }

    void CopyData(const void* src, size_t size, size_t offset = 0) {
        if constexpr (T == BufferType::Upload)
            memcpy(mMappedData + offset, src, size);
        else
            static_assert(T == BufferType::Upload, "ERROR: You can only call CopyData on an UPLOAD Buffer!!");
    }

    ID3D12Resource* GetResource() const { return mResource.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return mResource->GetGPUVirtualAddress(); }

private:
    ComPtr<ID3D12Resource> mResource;
    UINT8* mMappedData = nullptr;
    size_t mSize = 0;
};

using UploadBuffer = GPUBuffer<BufferType::Upload>;
using UAVBuffer = GPUBuffer<BufferType::UAV>;
