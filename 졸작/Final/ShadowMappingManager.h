#pragma once

class LightManager;

struct CascadeShadowConstants
{
	XMMATRIX lightVP[3];	// 3 cascade levels
	XMFLOAT4 cascadeSplit;	// 4 cascade ranges
	float shadowAmbientMin;	// IBL ambient 배율 하한 (shadow=0일 때), 1.0이면 중첩 없음
	float shadowFloor;		// shadow 값 하한, 0이면 원본 그대로, 0보다 크면 그림자 옅어짐
	float overheadMode;		// 1=실내 overhead 동적 그림자 모드(태양 CSM 대신), 0=일반 캐스케이드
	float overheadStrength;	// overhead 그림자 강도 (0=없음, 1=완전 어둠). 다른 맵과 눈으로 맞춤
	float overheadAmbientBoost;	// 실내 IBL ambient(채움광) 배율. 어두운 영역/얼굴 대비 완화
	XMFLOAT3 shadowPad2;
};

class ShadowMappingManager
{
public:
	void Initialize(ID3D12Device* device);
	void UpdateCascadeShadow(const XMFLOAT3& center);
	// 실내(성당/Final) 전용: 태양 CSM 대신 위에서 내려보는 ortho 그림자 1장 (동적 캐스터만)
	void UpdateOverheadShadow(const XMFLOAT3& center);
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

	// Overhead(실내) 튜닝 — 매 프레임 UpdateOverheadShadow에서 읽으므로 별도 업로드 불필요
	float& GetOverheadHalfSize() { return overheadHalfSize; }
	float& GetOverheadHeight() { return overheadHeight; }
	XMFLOAT3& GetOverheadLightDir() { return overheadLightDir; }
	bool& GetOverheadFollowNearestLight() { return overheadFollowNearestLight; }
	float& GetOverheadTilt() { return overheadTilt; }

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

	// Overhead(실내) 그림자 파라미터 — center(카메라 타겟) 기준으로 따라다님
	float overheadHalfSize = 40.0f;   // ortho 가로/세로 절반 (커버 범위)
	float overheadHeight = 80.0f;     // center→라이트 거리 = far 범위
	// 하향광 방향(정규화 전, 빛이 진행하는 방향). y가 지배적이라 거의 수직 + 살짝 사선.
	// x: +면 그림자 +X(우)로 / z: +면 그림자 +Z(앞)으로. World Z+가 앞방향.
	// follow가 켜지면 가장 가까운 point light 방향으로 대체되고, 이 값은 fallback.
	XMFLOAT3 overheadLightDir = { 0.22f, -1.0f, -0.22f };

	bool overheadFollowNearestLight = true;        // 가장 가까운 중심 바닥 조명 기준 방향
	float overheadTilt = 1.0f;                     // follow 시 최대 lean(=최대 그림자 길이) 클램프
	XMFLOAT2 overheadSmoothedLean = { 0.0f, 0.0f };// 조명 전환 튐 방지용 스무딩 상태(x,z)

	// 캐스케이드별 freeze/invalidation 상태 (texel-snap 인덱스 + sun 방향 비교용)
	XMINT2   lastSnap[CASCADE_COUNT];
	XMFLOAT3 lastSunDir;
	bool     cascadeDirty[CASCADE_COUNT];

	LightManager* lightMgr = nullptr;
};