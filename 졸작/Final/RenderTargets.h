#pragma once

class ShadowMappingManager;

class RenderTargets
{
public:
	void Initialize(ID3D12Device* device, ShadowMappingManager* shadowMgr);

	void AddSsaoSRV(ID3D12Device* device, ID3D12Resource* ssaoBlurRT);
	void RegisterHDRSceneToBindless(ID3D12Device* device);

	ID3D12Resource* GetDepthBuffer() const { return dsvBuffer.Get(); }
	ID3D12Resource* GetGBuffer(int index) const { return gBufferRT[index].Get(); }
	ID3D12Resource* GetFogRT() const { return fogRT.Get(); }
	ID3D12Resource* GetHDRSceneRT() const { return HDRSceneRT.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return dsvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferRTV(int index) const { return gBufferRTVHandles[index]; }
	D3D12_CPU_DESCRIPTOR_HANDLE* GetGBufferRTVArray() { return gBufferRTVHandles; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetFogRTV() const { return fogRTVHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetHDRSceneRTV() const { return HDRSceneRTVHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetHDRSceneSRV() const { return HDRSceneSRVHandle; }

	ID3D12DescriptorHeap* GetDeferredSRVHeap() const { return deferredSRVHeap.Get(); }
	ID3D12DescriptorHeap* GetHDRSceneSRVHeap() const { return HDRSceneSRVHeap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetPointShadowSRV() const { return pointShadowSRVHandle; }

private:
	void CreateDepthStencilBuffer(ID3D12Device* device);
	void CreateGBuffer(ID3D12Device* device);
	void CreateFogRenderTarget(ID3D12Device* device);
	void CreateDeferredRenderingDescriptors(ID3D12Device* device, ShadowMappingManager* shadowMgr);
	void CreateHDRSceneRenderTarget(ID3D12Device* device);

private:
	// Main DSV
	ComPtr<ID3D12Resource> dsvBuffer;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

	// G-Buffer
	ComPtr<ID3D12Resource> gBufferRT[3];
	ComPtr<ID3D12DescriptorHeap> gBufferRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE gBufferRTVHandles[3] = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gBufferSRVHandles[3] = {};

	// Fog
	ComPtr<ID3D12Resource> fogRT;
	ComPtr<ID3D12DescriptorHeap> fogRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE fogRTVHandle = {};

	// Deferred SRV Heap
	ComPtr<ID3D12DescriptorHeap> deferredSRVHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE pointShadowSRVHandle = {};

	// HDR Scene
	ComPtr<ID3D12Resource> HDRSceneRT;
	ComPtr<ID3D12DescriptorHeap> HDRSceneRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneRTVHandle = {};
	ComPtr<ID3D12DescriptorHeap> HDRSceneSRVHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE HDRSceneSRVHandle = {};
};
