#include "pch.h"
#include "DescriptorHeap.h"
#include "Device.h"

void DescriptorHeap::Initialize(ID3D12Device* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 2;  
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
    MASSERT(SUCCEEDED(hr), "Failed to create SRV heap");

    srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DescriptorHeap::CreateSRV(ID3D12Device* device, ID3D12Resource* texture, UINT index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    D3D12_RESOURCE_DESC resourceDesc = texture->GetDesc();

    srvDesc.Format = resourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

    device->CreateShaderResourceView(texture, &srvDesc, cpuHandle);
}

ID3D12DescriptorHeap* DescriptorHeap::GetSRVHeap() const
{
	return srvHeap.Get();
}

UINT DescriptorHeap::GetSRVHeapSize() const
{
	return srvDescriptorSize;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * srvDescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(UINT index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * srvDescriptorSize;
    return handle;
}
