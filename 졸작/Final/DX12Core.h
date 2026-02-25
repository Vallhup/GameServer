#pragma once

struct FrameConstants
{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX invViewProj;
	XMFLOAT3 cameraPosition;
	float time;
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
class RenderTargetManager;
class LightManager;
class RootSignature;
class Shader;

class DX12Core
{
public:
	void Initialize(HWND hwnd);

	//-------------------------------------------------------
	// Render line
	void RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void BeginShadowPass();
	void EndShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);

	void BeginGBufferPass();
	void EndGBufferPass();

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

	LightManager* GetLightMgr() { return lightMgr.get(); }
	RenderTargetManager* GetRenderTargetMgr() { return rtMgr.get(); }

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
	unique_ptr<RenderTargetManager> rtMgr;
	unique_ptr<LightManager> lightMgr;

	unique_ptr<RootSignature> rootSig;
	unique_ptr<Shader> shader;
	unique_ptr<UploadBuffer> frameCB;
	unique_ptr<UploadBuffer> sceneCB;
	unique_ptr<UploadBuffer> shadowFrameCB;
	unique_ptr<UploadBuffer> fogCB;

	XMFLOAT3 playerCurrentPos = { 0, 0, 0 };

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
