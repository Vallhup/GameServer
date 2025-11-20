#pragma once

class UAVBuffer
{
public:
	void Initialize(ID3D12Device* device, size_t sizeInBytes);

	ID3D12Resource* GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

private:
	ComPtr<ID3D12Resource> mResource;
	size_t mSize = 0;
};

