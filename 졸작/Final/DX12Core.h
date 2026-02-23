#pragma once

struct FrameConstants
{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX invViewProj;
	XMFLOAT3 cameraPosition;
	float padding;
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

class RootSignature;
class Shader;
class RenderTargetManager;
class LightManager;

class DX12Core
{
public:
	void Initialize(HWND hwnd);

	void BeginShadowPass();
	void EndShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);

	void BeginForwardPass();
	
	void BeginGBufferPass();
	void EndGBufferPass();

	void BeginLightingPass();
	
	void RenderFullscreenQuad();

	void RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void RenderEnd();
	void WaitSync();

	void FlushCommandQueue();
	void ResetCommandQueue();

	ID3D12Device* GetDevice() const;
	ID3D12CommandQueue* GetCmdQueue() const;
	ID3D12GraphicsCommandList* GetGraphicsCmdList() const;
	ID3D12GraphicsCommandList* GetLoadingCmdList() const;
	ID3D12GraphicsCommandList* GetActiveCmdList() const;

	void SetLoadingMode(bool loading);
	void ExecuteLoadingCommands();

	IDXGISwapChain4* GetSwapChain() const;
	RootSignature* GetRootSig() const;
	Shader* GetShader() const;
	UploadBuffer* GetFrameCB() const;
	UploadBuffer* GetSceneCB() const;
	UploadBuffer* GetFogCB() const;

	LightManager* GetLightMgr() { return lightMgr.get(); }
	RenderTargetManager* GetRenderTargetMgr() { return rtMgr.get(); }

	void SetBackgroundColor(const float* color);
	void SetPlayerPosForShadow(const XMFLOAT3& pos);

private:
	void CreateDevice();
	void CreateDXGI(HWND hwnd);
	void CreateCommandObjects();
	void CreateSwapChain(HWND hwnd);
	void CreateRenderTargetView();

private:
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory7> dxgi;

	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList> cmdList;

	ComPtr<ID3D12CommandAllocator> loadingCmdAlloc;
	ComPtr<ID3D12GraphicsCommandList> loadingCmdList;
	bool isLoadingMode = false;

	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = INVALID_HANDLE_VALUE;
	const float* backgroundColor = {};

	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> rtvBuffer[SWAP_CHAIN_BUFFER_COUNT];
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[SWAP_CHAIN_BUFFER_COUNT];
	UINT32 backBufferIndex = 0;

	unique_ptr<RootSignature> rootSig;
	unique_ptr<Shader> shader;
	unique_ptr<UploadBuffer> frameCB;
	unique_ptr<UploadBuffer> sceneCB;
	unique_ptr<UploadBuffer> shadowFrameCB;
	unique_ptr<UploadBuffer> fogCB;

	// Managers
	unique_ptr<RenderTargetManager> rtMgr;
	unique_ptr<LightManager> lightMgr;

	XMFLOAT3 playerCurrentPos = { 0, 0, 0 };
};
