#include "pch.h"
#include "FroxelManager.h"

void FroxelManager::Initialize(ID3D12Device* device)
{
	CreateFroxelVolume(device);
}

void FroxelManager::CreateFroxelVolume(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Width = FROXEL_WIDTH;
	texDesc.Height = FROXEL_HEIGHT;
	texDesc.DepthOrArraySize = FROXEL_DEPTH;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
		IID_PPV_ARGS(&froxelScattering));
	MASSERT(SUCCEEDED(hr), "Failed to create Froxel Scattering!!\n");

	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
		IID_PPV_ARGS(&froxelIntegrated));
	MASSERT(SUCCEEDED(hr), "Failed to create Froxel Integrated!!\n");

	OutputDebugStringA("Froxel Volume Created!!\n");

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 3;		// ScatteringUAV + IntegratedUAV + IntegratedSRV
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&froxelHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create froxel Heap");

	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = froxelHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = froxelHeap->GetGPUDescriptorHandleForHeapStart();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = FROXEL_DEPTH;
	device->CreateUnorderedAccessView(froxelScattering.Get(), nullptr, &uavDesc, cpuHandle);
	froxelScatteringUAV = gpuHandle;
	cpuHandle.ptr += srvSize;
	gpuHandle.ptr += srvSize;

	OutputDebugStringA("Froxel Scattering UAV Created!!\n");

	device->CreateUnorderedAccessView(froxelIntegrated.Get(), nullptr, &uavDesc, cpuHandle);
	froxelIntegratedUAV = gpuHandle;
	cpuHandle.ptr += srvSize;
	gpuHandle.ptr += srvSize;

	OutputDebugStringA("Froxel Integrated UAV Created!!\n");

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(froxelIntegrated.Get(), &srvDesc, cpuHandle);
	froxelIntegratedSRV = gpuHandle;

	OutputDebugStringA("Froxel Integrated SRV Created!!\n");
}