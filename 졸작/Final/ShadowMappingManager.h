#pragma once

struct CascadeShadowConstants
{
	XMMATRIX lightVP[4];	// 4 cascade levels
	XMFLOAT4 cascadeSplit;	// 4 cascade ranges
};

class ShadowMappingManager
{
public:
	void Initialize(ID3D12Device* device);
	void UpdateCascadeShadow(const XMFLOAT3& center);

	ID3D12Resource* GetCsmResource() const { return csmTexture.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCsmDSV(int index) const { return csmDSVHandle[index]; }
	int GetCascadeCount() const { return CASCADE_COUNT; }
	UINT GetShadowMapSize() const { return SHADOW_MAP_SIZE; }
	UploadBuffer* GetCsmCB() const { return csmConstantBuffer.get(); }

private:
	void SettingsForCSM();
	void CreateCSMResources(ID3D12Device* device);

	void CreateAtlasResources();

private:
	// Cascade shadow mapping
	static const int CASCADE_COUNT = 2;
	static const UINT SHADOW_MAP_SIZE = 2048;

	ComPtr<ID3D12Resource> csmTexture;
	ComPtr<ID3D12DescriptorHeap> csmDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE csmDSVHandle[CASCADE_COUNT];

	unique_ptr<UploadBuffer> csmConstantBuffer;
	CascadeShadowConstants csmConstants;
	XMVECTOR csmLightDir;

	// Shadow Atlas (추후)

};

// Cascade shadow mapping과 Shadow Atlas 둘다 관리하는 매니저
// 문제, shadow를 표현하기 위한 조명을 어떻게 관리할 것인가? - 고민중
// 일단, CSM은 조명 하나만 작용하니까 조명 위치 설정해서 구현