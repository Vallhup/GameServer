#include "pch.h"
#include "ClusterLightManager.h"

void ClusterLightManager::Initialize(ID3D12Device* device)
{
	paramsCB = make_unique<UploadBuffer>();
	lightIndexList = make_unique<UAVBuffer>();
	lightGrid = make_unique<UAVBuffer>();
	globalCounter = make_unique<UAVBuffer>();

	paramsCB->Initialize(device, sizeof(ClusterParamsConstants));
	lightIndexList->Initialize(device, sizeof(UINT) * LIGHT_INDEX_LIST_SIZE);    // ~4MB
	lightGrid->Initialize(device, sizeof(XMUINT2) * CLUSTER_COUNT);              // ~63KB
	globalCounter->Initialize(device, sizeof(UINT));                             // 4B (256B align)

	// 카메라 default (Camera.cpp의 zNear=1.0, zFar=3000.0과 일치)
	UpdateParams(1.0f, 3000.0f, static_cast<float>(WinSize.x), static_cast<float>(WinSize.y));

	// counter clear용 디스크립터 힙 (shader-visible 1, non-visible 1)
	D3D12_DESCRIPTOR_HEAP_DESC visDesc = {};
	visDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	visDesc.NumDescriptors = 1;
	visDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = device->CreateDescriptorHeap(&visDesc, IID_PPV_ARGS(&clearHeapVisible));
	MASSERT(SUCCEEDED(hr), "Failed to create cluster counter clear heap (visible)");

	D3D12_DESCRIPTOR_HEAP_DESC nonVisDesc = visDesc;
	nonVisDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&nonVisDesc, IID_PPV_ARGS(&clearHeapNonVisible));
	MASSERT(SUCCEEDED(hr), "Failed to create cluster counter clear heap (non-visible)");

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = 1;
	uavDesc.Buffer.StructureByteStride = sizeof(UINT);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(globalCounter->GetResource(), nullptr, &uavDesc,
		clearHeapVisible->GetCPUDescriptorHandleForHeapStart());
	device->CreateUnorderedAccessView(globalCounter->GetResource(), nullptr, &uavDesc,
		clearHeapNonVisible->GetCPUDescriptorHandleForHeapStart());

	counterUAV_GPU = clearHeapVisible->GetGPUDescriptorHandleForHeapStart();
	counterUAV_CPU_NonVisible = clearHeapNonVisible->GetCPUDescriptorHandleForHeapStart();
}

void ClusterLightManager::UpdateParams(float zNear, float zFar, float screenW, float screenH)
{
	paramsData.gridDims = { GRID_X, GRID_Y, GRID_Z };
	paramsData.zNear = zNear;
	paramsData.zFar = zFar;

	float logRatio = logf(zFar / zNear);
	paramsData.sliceScale = static_cast<float>(GRID_Z) / logRatio;
	paramsData.sliceBias = -static_cast<float>(GRID_Z) * logf(zNear) / logRatio;

	paramsData.screenSize = { screenW, screenH };
	paramsData.pad0 = 0.0f;
	paramsData.pad1 = { 0.0f, 0.0f };

	paramsCB->CopyData(&paramsData, sizeof(ClusterParamsConstants));
}

void ClusterLightManager::ClearCounter(ID3D12GraphicsCommandList* cmd)
{
	ID3D12DescriptorHeap* heaps[] = { clearHeapVisible.Get() };
	cmd->SetDescriptorHeaps(1, heaps);

	UINT clearVal[4] = { 0, 0, 0, 0 };
	cmd->ClearUnorderedAccessViewUint(
		counterUAV_GPU,
		counterUAV_CPU_NonVisible,
		globalCounter->GetResource(),
		clearVal,
		0, nullptr
	);
}
