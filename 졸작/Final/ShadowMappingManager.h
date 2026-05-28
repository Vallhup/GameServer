#pragma once

class LightManager;

struct CascadeShadowConstants
{
	XMMATRIX lightVP[3];	// 3 cascade levels
	XMFLOAT4 cascadeSplit;	// 4 cascade ranges
	float shadowAmbientMin;	// IBL ambient 배율 하한 (shadow=0일 때), 1.0이면 중첩 없음
	float shadowFloor;		// shadow 값 하한, 0이면 원본 그대로, 0보다 크면 그림자 옅어짐
	XMFLOAT2 shadowPad;
};

class ShadowMappingManager
{
public:
	void Initialize(ID3D12Device* device);
	void UpdateCascadeShadow(const XMFLOAT3& center);
	void SetLightMgr(LightManager* lm) { lightMgr = lm; }
	void Invalidate();

	ID3D12Resource* GetCsmResource() const { return csmTexture.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCsmDSV(int index) const { return csmDSVHandle[index]; }
	int GetCascadeCount() const { return CASCADE_COUNT; }
	UINT GetShadowMapSize() const { return SHADOW_MAP_SIZE; }
	UploadBuffer* GetCsmCB() const { return csmConstantBuffer.get(); }
	bool IsCascadeDirty(int i) const { return cascadeDirty[i]; }

	// For ImGui
	CascadeShadowConstants& GetCsmConstants() { return csmConstants; }
	void UploadCsmConstants() { csmConstantBuffer->CopyData(&csmConstants, sizeof(CascadeShadowConstants)); }

	// Static caster cache (cascade 2 전용, single slice)
	ID3D12Resource* GetStaticCsmResource() const { return staticCsmTexture.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetStaticCsmDSV() const { return staticCsmDSVHandle; }
	int GetStaticCacheCascadeIndex() const { return STATIC_CACHE_CASCADE_INDEX; }

private:
	void SettingsForCSM();
	void CreateCSMResources(ID3D12Device* device);
	void CreateStaticCSMResources(ID3D12Device* device);

	void CreateAtlasResources();

private:
	// Cascade shadow mapping
	static const int CASCADE_COUNT = 3;
	static const UINT SHADOW_MAP_SIZE = 4096;
	static constexpr int STATIC_CACHE_CASCADE_INDEX = 2;

	ComPtr<ID3D12Resource> csmTexture;
	ComPtr<ID3D12DescriptorHeap> csmDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE csmDSVHandle[CASCADE_COUNT];

	// Cascade 2 전용 static caster depth 캐시
	ComPtr<ID3D12Resource> staticCsmTexture;
	ComPtr<ID3D12DescriptorHeap> staticCsmDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE staticCsmDSVHandle = {};

	unique_ptr<UploadBuffer> csmConstantBuffer;
	CascadeShadowConstants csmConstants;
	XMVECTOR csmLightDir;

	// 캐스케이드별 freeze/invalidation 상태 (texel-snap 인덱스 + sun 방향 비교용)
	XMINT2   lastSnap[CASCADE_COUNT];
	XMFLOAT3 lastSunDir;
	bool     cascadeDirty[CASCADE_COUNT];

	LightManager* lightMgr = nullptr;
};