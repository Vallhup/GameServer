#include "pch.h"
#include "UAVBuffer.h"

void UAVBuffer::Initialize(ID3D12Device* device, size_t sizeInBytes)
{
	mSize = (sizeInBytes + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(
		mSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mResource)
	);
	MASSERT(SUCCEEDED(hr), "Failed to create UAVBuffer resource");
}

ID3D12Resource* UAVBuffer::GetResource() const
{
	return mResource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS UAVBuffer::GetGPUVirtualAddress() const
{
	return mResource->GetGPUVirtualAddress();
}