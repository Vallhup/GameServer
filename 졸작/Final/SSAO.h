#pragma once

struct SSAOConstants 
{
	XMFLOAT4 samples[16];
	XMFLOAT2 noiseScale;
	float samplingRadius;
	float padding;
};

class RenderTargets;

class SSAO
{
public:
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderTargets* rt);

	ID3D12Resource* GetSsaoRT() const { return ssaoRT.Get(); }
	ID3D12Resource* GetSsaoBlurRT() const { return ssaoBlurRT.Get(); }
	ID3D12DescriptorHeap* GetSsaoSRVHeap() const { return ssaoSRVHeap.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSsaoRTVHandle() const { return ssaoRTVHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSsaoBlurRTVHandle() const { return ssaoBlurRTVHandle; }
	UploadBuffer* GetSsaoCB() const { return ssaoCB.get(); }

private:
	void CreateSSAOResources(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderTargets* rt);
	void GenerateSampleKernel(ID3D12Device* device);
	void GenerateNoiseTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void CreateSRVs(ID3D12Device* device, RenderTargets* rt);

private:
	ComPtr<ID3D12Resource> ssaoRT;
	ComPtr<ID3D12Resource> ssaoBlurRT;
	ComPtr<ID3D12Resource> noiseTexture;
	ComPtr<ID3D12Resource> noiseUploadBuffer;

	ComPtr<ID3D12DescriptorHeap> ssaoRTVHeap;
	ComPtr<ID3D12DescriptorHeap> ssaoSRVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoRTVHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoBlurRTVHandle = {};

	SSAOConstants ssaoConstant;
	unique_ptr<UploadBuffer> ssaoCB;
};
