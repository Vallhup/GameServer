#pragma once

class DepthStencilBuffer
{
public:
	void Initialize(ID3D12Device* device, DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT);

	D3D12_CPU_DESCRIPTOR_HANDLE	GetDSVCpuHandle();
	DXGI_FORMAT GetDSVFormat();

private:
	ComPtr<ID3D12Resource>				dsvbuffer;
	ComPtr<ID3D12DescriptorHeap>		dsvheap;
	D3D12_CPU_DESCRIPTOR_HANDLE			dsvhandle = {};
	DXGI_FORMAT							dsvformat = {};
};