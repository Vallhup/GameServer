#include "pch.h"
#include "ReadBackBuffer.h"

void ReadBackBuffer::Initialize(ID3D12Device* device, UINT sizeInBytes)
{
    mSize = sizeInBytes;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&mResource)
    );
    MASSERT(SUCCEEDED(hr), "Failed to create ReadBack buffer");
}

void ReadBackBuffer::CopyFromGPU(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* srcResource)
{
    // UAV ¡æ Copy Source
    D3D12_RESOURCE_BARRIER barrierBefore = CD3DX12_RESOURCE_BARRIER::Transition(
        srcResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE
    );
    cmdList->ResourceBarrier(1, &barrierBefore);

    // GPU ¡æ ReadBack ¹öÆÛ
    cmdList->CopyResource(mResource.Get(), srcResource);

    // Copy Source ¡æ UAV
    D3D12_RESOURCE_BARRIER barrierAfter = CD3DX12_RESOURCE_BARRIER::Transition(
        srcResource,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    cmdList->ResourceBarrier(1, &barrierAfter);
}

void ReadBackBuffer::ReadData(void* dest, size_t size)
{
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, mSize };

    HRESULT hr = mResource->Map(0, &readRange, &mappedData);
    MASSERT(SUCCEEDED(hr), "Failed to map ReadBack buffer");

    memcpy(dest, mappedData, size);

    D3D12_RANGE writeRange = { 0, 0 };
    mResource->Unmap(0, &writeRange);
}

void ReadBackBuffer::Release()
{
    mResource.Reset();
}