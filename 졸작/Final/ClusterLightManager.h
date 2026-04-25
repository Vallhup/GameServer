#pragma once

struct ClusterParamsConstants
{
	XMUINT3 gridDims;
	float zNear;
	float zFar;
	float sliceScale;       // numSlices / log(far/near)
	float sliceBias;        // -numSlices * log(near) / log(far/near)
	float pad0;
	XMFLOAT2 screenSize;
	XMFLOAT2 pad1;
};

class ClusterLightManager
{
public:
	static constexpr UINT GRID_X = 24;
	static constexpr UINT GRID_Y = 14;
	static constexpr UINT GRID_Z = 24;
	static constexpr UINT CLUSTER_COUNT = GRID_X * GRID_Y * GRID_Z;            // 8064
	static constexpr UINT MAX_LIGHTS_PER_CLUSTER = 128;
	static constexpr UINT LIGHT_INDEX_LIST_SIZE = CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER;

	void Initialize(ID3D12Device* device);
	void UpdateParams(float zNear, float zFar, float screenW, float screenH);
	void ClearCounter(ID3D12GraphicsCommandList* cmd);

	UploadBuffer* GetParamsCB() const { return paramsCB.get(); }
	UAVBuffer* GetLightIndexList() const { return lightIndexList.get(); }
	UAVBuffer* GetLightGrid() const { return lightGrid.get(); }
	UAVBuffer* GetGlobalCounter() const { return globalCounter.get(); }

private:
	ClusterParamsConstants paramsData = {};

	unique_ptr<UploadBuffer> paramsCB;
	unique_ptr<UAVBuffer> lightIndexList;
	unique_ptr<UAVBuffer> lightGrid;
	unique_ptr<UAVBuffer> globalCounter;

	// ClearUnorderedAccessViewUint 용 디스크립터 힙 (shader-visible + non-visible 둘 다 필요)
	ComPtr<ID3D12DescriptorHeap> clearHeapVisible;
	ComPtr<ID3D12DescriptorHeap> clearHeapNonVisible;
	D3D12_GPU_DESCRIPTOR_HANDLE counterUAV_GPU = {};
	D3D12_CPU_DESCRIPTOR_HANDLE counterUAV_CPU_NonVisible = {};
};
