#pragma once

struct FrameConstants
{
	XMMATRIX view;
	XMMATRIX projection;
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

struct LightData {
	XMFLOAT3 position;    // Point light용 (directional일 때는 direction)
	float range;          // Point light 범위
	XMFLOAT3 color;
	float intensity;
	int type;             // 0=directional, 1=point
	XMFLOAT3 padding;
};

struct DeferredLightConstants {
	int lightCount;
	XMFLOAT3 padding;
	LightData lights[23]; // 조명 60개부터 렉걸린다 이유 해결 안됨
};

struct ForwardLightConstants {
	XMFLOAT3 direction;
	float padding;
	XMFLOAT3 color;
	float intensity;
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

class DX12Core
{
public:
	void Initialize(HWND hwnd);

	void BeginShadowPass();
	void EndShadowPass();

	void BeginForwardPass();
	
	void BeginGBufferPass();
	void EndGBufferPass();

	void BeginLightingPass();
	
	void UpdateLights();
	void RenderFullscreenQuad();

	void RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void RenderEnd();
	void WaitSync();

	void FlushCommandQueue();
	void ResetCommandQueue();

	ID3D12Device* GetDevice() const;
	ID3D12CommandQueue* GetCmdQueue() const;
	ID3D12GraphicsCommandList* GetGraphicsCmdList() const;
	IDXGISwapChain4* GetSwapChain() const;
	RootSignature* GetRootSig() const;
	Shader* GetShader() const;
	UploadBuffer* GetFrameCB() const;
	UploadBuffer* GetSceneCB() const;
	UploadBuffer* GetDeferredLightCB() const;
	UploadBuffer* GetForwardLightCB() const;
	UploadBuffer* GetFogCB() const;

	ID3D12DescriptorHeap* GetDeferredSRVHeap() const;

	DeferredLightConstants& GetDeferredLightData() { return deferredLightData; }
	ForwardLightConstants& GetForwardLightData() { return forwardLightData; }

	void SetBackgroundColor(const float* color);
	void SetPlayerPosForShadow(const XMFLOAT3& pos);

private:
	void CreateDevice();
	void CreateDXGI(HWND hwnd);
	void CreateCommandObjects();
	void CreateSwapChain(HWND hwnd);
	void CreateRenderTargetView();
	void CreateDepthStencilBuffer(DXGI_FORMAT dsvformat = DXGI_FORMAT_D32_FLOAT);

	void CreateGBuffer();
	void CreateShadowMap();
	void CreateDeferredRenderingDescriptors();

	void SetupLights();

private:
	// 고정
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory7> dxgi;

	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = INVALID_HANDLE_VALUE;
	const float* backgroundColor = {};

	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> rtvBuffer[SWAP_CHAIN_BUFFER_COUNT];
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[SWAP_CHAIN_BUFFER_COUNT];
	UINT32 backBufferIndex = 0;

	ComPtr<ID3D12Resource> dsvBuffer;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	DXGI_FORMAT dsvFormat = {};

	// deferred rendering
	ComPtr<ID3D12Resource> gBufferRT[4];
	ComPtr<ID3D12DescriptorHeap> gBufferRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE gBufferRTVHandles[4];
	D3D12_GPU_DESCRIPTOR_HANDLE gBufferSRVHandles[4];

	ComPtr<ID3D12DescriptorHeap> deferredSRVHeap;

	bool useDeferredRendering = true;

	// 변경 가능
	unique_ptr<RootSignature> rootSig;
	unique_ptr<Shader> shader;
	unique_ptr<UploadBuffer> frameCB;
	unique_ptr<UploadBuffer> sceneCB;
	unique_ptr<UploadBuffer> deferredLightCB;
	unique_ptr<UploadBuffer> forwardLightCB;
	unique_ptr<UploadBuffer> shadowFrameCB;
	unique_ptr<UploadBuffer> fogCB;

	// Shadow Mapping resources
	ComPtr<ID3D12Resource> shadowMapTexture;
	ComPtr<ID3D12DescriptorHeap> shadowMapDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE shadowMapDSVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRVHandle;

	static const UINT SHADOW_MAP_SIZE = 2048;

	XMFLOAT3 playerCurrentPos = { 0, 0, 0 };
	DeferredLightConstants deferredLightData = {};
	ForwardLightConstants forwardLightData = {};
};
