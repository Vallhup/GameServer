#pragma once

class SSAO
{
public:
	void Initialize(ID3D12Device* device);

	ID3D12Resource* GetSsaoRT() const { return ssaoRT.Get(); }
	ID3D12Resource* GetSsaoBlurRT() const { return ssaoBlurRT.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSsaoRTVHandle() const { return ssaoRTVHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSsaoBlurRTVHandle() const { return ssaoBlurRTVHandle; }

private:
	void CreateSSAOResources(ID3D12Device* device);

private:
	ComPtr<ID3D12Resource> ssaoRT;
	ComPtr<ID3D12Resource> ssaoBlurRT;
	ComPtr<ID3D12Resource> noiseTexture;
	ComPtr<ID3D12DescriptorHeap> ssaoRTVHeap;
	ComPtr<ID3D12DescriptorHeap> ssaoSRVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoRTVHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoBlurRTVHandle = {};
};
