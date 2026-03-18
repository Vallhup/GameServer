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
};

struct ObjectConstants
{
	XMMATRIX world;
	int useTexture;
	int useInstancing;
	UINT materialIndex;
	int padding;
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

class DeviceContext;
class SwapChain;
class ShadowMappingManager;
class RenderTargets;
class LightManager;
class FroxelManager;
class SSAO;
class RootSignature;
class Shader;

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

	void BeginGBufferPass();
	void EndGBufferPass();

	void BeginSsaoPass();
	void EndSsaoPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void BeginSsaoBlurPass();
	void EndSsaoBlurPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void ClearSsaoRT();

	void BeginLightingPass();
	void RenderFullscreenQuad();

	void BeginForwardPass();
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
	SSAO* GetSsaoMgr() { return ssaoMgr.get(); }

	IDXGISwapChain4* GetSwapChain() const;
	RootSignature* GetRootSig() const;
	Shader* GetShader() const;
	UploadBuffer* GetFrameCB() const;
	UploadBuffer* GetSceneCB() const;
	UploadBuffer* GetFogCB() const;

	void SetPlayerPosForShadow(const XMFLOAT3& pos);

private:
	// Managers
	unique_ptr<DeviceContext> deviceCtx;
	unique_ptr<SwapChain> swapChainMgr;
	unique_ptr<ShadowMappingManager> shadowMgr;
	unique_ptr<RenderTargets> rtMgr;
	unique_ptr<LightManager> lightMgr;
	unique_ptr<FroxelManager> froxelMgr;
	unique_ptr<SSAO> ssaoMgr;

	unique_ptr<RootSignature> rootSig;
	unique_ptr<Shader> shader;
	unique_ptr<UploadBuffer> frameCB;
	unique_ptr<UploadBuffer> sceneCB;
	unique_ptr<UploadBuffer> fogCB;

	XMFLOAT3 playerCurrentPos = { 0, 0, 0 };

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
