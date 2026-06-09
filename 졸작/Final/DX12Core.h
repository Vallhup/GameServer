#pragma once

struct FrameConstants
{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX invViewProj;
	XMFLOAT3 cameraPosition;
	float time;
	UINT lutIndex;
	UINT prevLutIndex;
	float lutBlendFactor;
	float saturationFactor;
	float screenBrightness;
};

struct ObjectConstants
{
	XMMATRIX world;
	int useTexture;
	int useInstancing;
	UINT materialIndex;
	int useVertexAnim = 0;
	int useTerrainBlend = 0;
	UINT splatmap1Index = 0xFFFFFFFF;
	UINT splatmap2Index = 0xFFFFFFFF;
	float splatUVScale = 0.0f;
	int splatLayerCount = 0;
	float dissolveAmount = 0.0f;          // 0=온전, 1=완전 분해
	UINT dissolveNoiseIndex = 0xFFFFFFFF; // bindless 노이즈 인덱스(미사용 시 0xFFFFFFFF)
	float brightness = 1.0f;              // unlit 정점색에 곱하는 밝기(1=원색)
};

struct FogConstants
{
	XMFLOAT4 fogColor;
	float fogStart;
	float fogRange;
	float fogZoneStart;
	float fogZoneEnd;
	float fogZoneFade;
	XMFLOAT3 fogPadding;
};

struct VolumetricFogConstants
{
	float density;           // 안개 밀도
	float scattering;        // 산란 계수
	float absorption;        // 흡수 계수
	float hgAnisotropy;      // Phase function g값

	int maxSteps;            // Ray March 스텝 수
	float maxDistance;       // 최대 거리
	float jitterStrength;    // Banding 완화용
	float heightFalloff;     // 높이 감쇠 계수

	float groundHeight;      // 기준 높이
	XMFLOAT3 lightColor;     // 안개 속 빛 색상

	float lightIntensity;    // 빛 강도
	XMFLOAT2 texelSize;
	float vfPadding;
};

class DeviceContext;
class SwapChain;
class ShadowMappingManager;
class RenderTargets;
class LightManager;
class FroxelManager;
class ClusterLightManager;
class SSAO;
class LookUpTextures;
class RootSignature;
class Shader;
class BloomManager;

class DX12Core
{
public:
	void Initialize(HWND hwnd);

	void Update();

	//-------------------------------------------------------
	// Render line
	void RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);

	void BeginShadowPass(int cascadeIdx);
	void EndShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, int cascadeIdx);
	void BeginStaticShadowPass();
	void EndStaticShadowPass();
	void CopyStaticToCsmCascade2();
	void BeginDynamicShadowPass();
	void EndDynamicShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void BeginOverheadShadowPass();
	void EndOverheadShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);

	void BeginGBufferPass();
	void EndGBufferPass();

	void SsaoPass();
	void SsaoBlurPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void ClearSsaoRT();

	void FogPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);

	void ClusterLightCullPass();
	void LightingPass();
	void ForwardPass();

	void BloomPass();

	void BlitPass();

	void RenderEnd();
	//-------------------------------------------------------

	//-------------------------------------------------------
	// Device Context helper
	ID3D12Device* GetDevice() const;
	ID3D12CommandQueue* GetCmdQueue() const;
	ID3D12GraphicsCommandList* GetGraphicsCmdList() const;
	ID3D12GraphicsCommandList* GetActiveCmdList() const;

	void SetLoadingMode(bool loading);
	void ExecuteLoadingCommands();
	void WaitSync();
	void FlushCommandQueue();
	void ResetCommandQueue();
	//-------------------------------------------------------

	ShadowMappingManager* GetShadowMgr() { return shadowMgr.get(); }
	RenderTargets* GetRenderTargetMgr() { return rtMgr.get(); }
	LightManager* GetLightMgr() { return lightMgr.get(); }
	FroxelManager* GetFroxelMgr() { return froxelMgr.get(); }
	ClusterLightManager* GetClusterLightMgr() { return clusterLightMgr.get(); }
	SSAO* GetSsaoMgr() { return ssaoMgr.get(); }
	LookUpTextures* GetLUTMgr() { return lutMgr.get(); }
	SwapChain* GetSwapChainMgr() { return swapChainMgr.get(); }
	BloomManager* GetBloomMgr() { return bloomMgr.get(); }

	IDXGISwapChain4* GetSwapChain() const;
	RootSignature* GetRootSig() const;
	Shader* GetShader() const;
	UploadBuffer* GetFrameCB() const;
	UploadBuffer* GetSceneCB() const;
	UploadBuffer* GetFogCB() const;
	UploadBuffer* GetVolumetricFogCB() const;

	VolumetricFogConstants& GetVolumetricFogData() { return volumetricFogData; }
	void UpdateVolumetricFog();

	void SetPlayerPosForShadow(const XMFLOAT3& pos);

private:
	// Managers
	unique_ptr<DeviceContext> deviceCtx;
	unique_ptr<SwapChain> swapChainMgr;
	unique_ptr<ShadowMappingManager> shadowMgr;
	unique_ptr<RenderTargets> rtMgr;
	unique_ptr<LightManager> lightMgr;
	unique_ptr<FroxelManager> froxelMgr;
	unique_ptr<ClusterLightManager> clusterLightMgr;
	unique_ptr<SSAO> ssaoMgr;
	unique_ptr<LookUpTextures> lutMgr;

	unique_ptr<BloomManager> bloomMgr;

	unique_ptr<RootSignature> rootSig;
	unique_ptr<Shader> shader;
	unique_ptr<UploadBuffer> frameCB;
	unique_ptr<UploadBuffer> sceneCB;
	unique_ptr<UploadBuffer> fogCB;
	unique_ptr<UploadBuffer> volumetricFogCB;

	VolumetricFogConstants volumetricFogData = {
		0.02f,                  // density
		0.8f,                   // scattering
		0.1f,                   // absorption
		0.6f,                   // hgAnisotropy
		32,                     // maxSteps
		160.0f,                 // maxDistance
		0.5f,                   // jitterStrength
		0.001f,                 // heightFalloff
		3.0f,                   // groundHeight
		{ 1.0f, 1.0f, 1.0f },   // lightColor
		1.0f,                   // lightIntensity
		{ 0.0f, 0.0f },
		0.0f    // padding
	};

	XMFLOAT3 playerCurrentPos = { 0, 0, 0 };

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
