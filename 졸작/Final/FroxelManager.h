#pragma once

class FroxelManager
{
public:
	void Initialize(ID3D12Device* device);

private:
	void CreateFroxelVolume(ID3D12Device* device);

private:
	ComPtr<ID3D12Resource> froxelScattering;
	ComPtr<ID3D12Resource> froxelIntegrated;

	ComPtr<ID3D12DescriptorHeap> froxelHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE froxelScatteringUAV;
	D3D12_GPU_DESCRIPTOR_HANDLE froxelIntegratedUAV;
	D3D12_GPU_DESCRIPTOR_HANDLE froxelIntegratedSRV;

	static constexpr UINT FROXEL_WIDTH = 160;
	static constexpr UINT FROXEL_HEIGHT = 90;
	static constexpr UINT FROXEL_DEPTH = 64;
};

// 일단, UAV기반 3D Texture 만들었고..
// 음.. 이제 뭘 어떻게 해야할지 모르겟..

// UAV & SRV 는 고유 HEAP이 필요하고, HANDLE이 필요해서 설정.
// 이제 뭐해야하지 ? - 어떻게 Froxel화 하지? 흠..