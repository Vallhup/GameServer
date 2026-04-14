#pragma once

struct CascadeShadowConstants
{
	XMMATRIX lightVP[2];	// 2 cascade levels
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
	static const UINT SHADOW_MAP_SIZE = 4096;

	ComPtr<ID3D12Resource> csmTexture;
	ComPtr<ID3D12DescriptorHeap> csmDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE csmDSVHandle[CASCADE_COUNT];

	unique_ptr<UploadBuffer> csmConstantBuffer;
	CascadeShadowConstants csmConstants;
	XMVECTOR csmLightDir;

	// Shadow Atlas (����)

};

// Cascade shadow mapping�� Shadow Atlas �Ѵ� �����ϴ� �Ŵ���
// ����, shadow�� ǥ���ϱ� ���� ������ ��� ������ ���ΰ�? - ������
// �ϴ�, CSM�� ���� �ϳ��� �ۿ��ϴϱ� ���� ��ġ �����ؼ� ����