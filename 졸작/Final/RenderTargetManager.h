#pragma once

class RenderTargetManager
{
public:
	void Initialize(ID3D12Device* device);

	ID3D12Resource* GetDepthBuffer() const { return dsvBuffer.Get(); }
	ID3D12Resource* GetGBuffer(int index) const { return gBufferRT[index].Get(); }
	ID3D12Resource* GetShadowMap() const { return shadowMapTexture.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return dsvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferRTV(int index) const { return gBufferRTVHandles[index]; }
	D3D12_CPU_DESCRIPTOR_HANDLE* GetGBufferRTVArray() { return gBufferRTVHandles; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetShadowMapDSV() const { return shadowMapDSVHandle; }

	ID3D12DescriptorHeap* GetDeferredSRVHeap() const { return deferredSRVHeap.Get(); }

	UINT GetShadowMapSize() const { return SHADOW_MAP_SIZE; }

private:
	void CreateDepthStencilBuffer(ID3D12Device* device);
	void CreateGBuffer(ID3D12Device* device);
	void CreateShadowMap(ID3D12Device* device);
	void CreateDeferredRenderingDescriptors(ID3D12Device* device);

private:
	// Main DSV
	ComPtr<ID3D12Resource> dsvBuffer;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

	// G-Buffer
	ComPtr<ID3D12Resource> gBufferRT[3];
	ComPtr<ID3D12DescriptorHeap> gBufferRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE gBufferRTVHandles[3];
	D3D12_GPU_DESCRIPTOR_HANDLE gBufferSRVHandles[3];

	// Shadow Map
	ComPtr<ID3D12Resource> shadowMapTexture;
	ComPtr<ID3D12DescriptorHeap> shadowMapDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE shadowMapDSVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRVHandle;

	// Deferred SRV Heap
	ComPtr<ID3D12DescriptorHeap> deferredSRVHeap;

	static const UINT SHADOW_MAP_SIZE = 4096;
};