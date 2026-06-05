#pragma once
#include "Material.h"

struct BloomRootConstants
{
	UINT srcMipIndex;
	float filterRadius;
	float texelSizeX;
	float texelSizeY;
	float intensity;
	float threshold;
	float knee;
	UINT isFirstPass;
};

class BloomManager
{
public:
	void Initialize(ID3D12Device* device);
	void RegisterMipsToBindless(ID3D12Device* device);

	ID3D12Resource* GetBloomTexture() const { return bloomTex.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetMipRTV(UINT mipLevel) const { return mipRTVHandles[mipLevel]; }

	UINT GetMipWidth(UINT mipLevel) const { return mipWidths[mipLevel]; }
	UINT GetMipHeight(UINT mipLevel) const { return mipHeights[mipLevel]; }

	float GetIntensity() const { return intensity; }
	void SetIntensity(float v) { intensity = v; }

	static constexpr UINT CHAIN_LENGTH = Material::BLOOM_CHAIN_LENGTH;

private:
	void CreateBloomResource(ID3D12Device* device);
	void CreateMipRTVs(ID3D12Device* device);

private:
	ComPtr<ID3D12Resource> bloomTex;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE mipRTVHandles[CHAIN_LENGTH] = {};
	UINT mipWidths[CHAIN_LENGTH] = {};
	UINT mipHeights[CHAIN_LENGTH] = {};

	float intensity = 0.1f;
};
