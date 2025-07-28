#pragma once

class DescriptorHeap
{
public:
	void Initialize(ID3D12Device* device);

	void CreateSRV(ID3D12Device* device, ID3D12Resource* texture, UINT index);

	ID3D12DescriptorHeap* GetSRVHeap() const;
	UINT GetSRVHeapSize() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const;

private:
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	UINT srvDescriptorSize;
};

