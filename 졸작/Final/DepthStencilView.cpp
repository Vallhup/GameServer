#include "pch.h"
#include "DepthStencilView.h"
#include "Engine.h"

void DepthStencilBuffer::Initialize(ID3D12Device* device, DXGI_FORMAT dsvFormat)
{
	dsvformat = dsvFormat;

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(dsvformat, WinSize.x, WinSize.y);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimizedClearValue = CD3DX12_CLEAR_VALUE(dsvformat, 1.0f, 0);

	device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&dsvbuffer));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvheap));

	dsvhandle = dsvheap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(dsvbuffer.Get(), nullptr, dsvhandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBuffer::GetDSVCpuHandle()
{
	return dsvhandle;
}

DXGI_FORMAT DepthStencilBuffer::GetDSVFormat()
{
	return dsvformat;
}
