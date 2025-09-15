#include "pch.h"
#include "DX12Core.h"
#include "RootSignature.h"
#include "Shader.h"
#include "Timer.h"

void DX12Core::Initialize(HWND hwnd)
{
	CreateDXGI(hwnd);
	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain(hwnd);
	CreateRenderTargetView();

	rootSig = make_unique<RootSignature>();
	shader = make_unique<Shader>();
	frameCB = make_unique<UploadBuffer>();
	sceneCB = make_unique<UploadBuffer>();
	deferredLightCB = make_unique<UploadBuffer>();
	forwardLightCB = make_unique<UploadBuffer>();
	shadowFrameCB = make_unique<UploadBuffer>();

	rootSig->Initialize(GetDevice());
	shader->InitializeForwardShader(GetDevice(), GetRootSig()->Get(), L"ForwardVS.hlsli", L"ForwardPS.hlsli");
	shader->InitializeGBufferShader(GetDevice(), GetRootSig()->Get(), L"GBufferVS.hlsli", L"GBufferPS.hlsli");
	shader->InitializeLightingShader(GetDevice(), GetRootSig()->Get(), L"FullscreenVS.hlsli", L"LightingPS.hlsli");
	shader->InitializeComputeShader(GetDevice(), GetRootSig()->Get(), L"Animation.hlsli");
	shader->InitializeShadowShader(GetDevice(), GetRootSig()->Get(), L"ShadowVS.hlsli", L"ShadowPS.hlsli");
	frameCB->Initialize(GetDevice(), sizeof(XMMATRIX) * 2);
	sceneCB->Initialize(GetDevice(), 256 * 1000);
	deferredLightCB->Initialize(GetDevice(), sizeof(DeferredLightConstants));
	forwardLightCB->Initialize(GetDevice(), sizeof(ForwardLightConstants));

	CreateDepthStencilBuffer();
	CreateGBuffer();
	CreateShadowMap();
}

void DX12Core::CreateDevice()
{
	HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
	MASSERT(SUCCEEDED(hr), "Failed to create D3D12 device");
}

void DX12Core::CreateDXGI(HWND hwnd)
{
	ASSERT(hwnd != nullptr);

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		RELEASE_COM(debugController);
	}
#endif

	HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgi));
	MASSERT(SUCCEEDED(hr), "Failed to create DXGI factory");
}

void DX12Core::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC desc = {
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue));
    MASSERT(SUCCEEDED(hr), "Failed to create Command Queue");

    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    MASSERT(SUCCEEDED(hr), "Failed to create Command Allocator");

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    MASSERT(SUCCEEDED(hr), "Failed to create Command List");

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    MASSERT(SUCCEEDED(hr), "Failed to create Fence");

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void DX12Core::CreateSwapChain(HWND hwnd)
{
	ComPtr<IDXGISwapChain> tempSwapChain;

	ComPtr<IDXGIAdapter> adapter;
	HRESULT hr = dxgi->EnumAdapters(0, &adapter);
	MASSERT(SUCCEEDED(hr), "Failed to get adapter!");

	ComPtr<IDXGIOutput> output;
	hr = adapter->EnumOutputs(0, &output);
	MASSERT(SUCCEEDED(hr), "Failed to get output!");

	UINT numModes = 0;
	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr);

	std::vector<DXGI_MODE_DESC> displayModes(numModes);
	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModes.data());

	DXGI_MODE_DESC bestMode = {};
	float bestRefreshRate = 0.0f;

	OutputDebugStringA("=== Matching Modes ===\n");

	for (const auto& mode : displayModes) {
		if (mode.Width == WinSize.x && mode.Height == WinSize.y) {
			float refreshRate = static_cast<float>(mode.RefreshRate.Numerator) / mode.RefreshRate.Denominator;

			string matchInfo = "Match found: " + std::to_string(mode.Width) + "x" + std::to_string(mode.Height) +
				" @ " + std::to_string(refreshRate) + "Hz (" +
				std::to_string(mode.RefreshRate.Numerator) + "/" +
				std::to_string(mode.RefreshRate.Denominator) + ")\n";
			OutputDebugStringA(matchInfo.c_str());

			if (refreshRate > bestRefreshRate) {
				bestMode = mode;
				bestRefreshRate = refreshRate;
				OutputDebugStringA("  -> New best mode selected!\n");
			}
			else {
				OutputDebugStringA("  -> Lower refresh rate, skipped\n");
			}
		}
	}

	if (bestRefreshRate == 0.0f) {
		OutputDebugStringA("No matching mode found! Using default 60Hz\n");
		bestMode.RefreshRate.Numerator = 60;
		bestMode.RefreshRate.Denominator = 1;
		bestMode.Width = WinSize.x;
		bestMode.Height = WinSize.y;
		bestRefreshRate = 60.0f;
	}

	OutputDebugStringA("=== FINAL SELECTION ===\n");
	OutputDebugStringA(("Selected refresh rate: " + std::to_string(bestRefreshRate) + "Hz (" +
		std::to_string(bestMode.RefreshRate.Numerator) + "/" +
		std::to_string(bestMode.RefreshRate.Denominator) + ")\n").c_str());

	GET(Timer).SetTargetFPS(bestRefreshRate);

	DXGI_SWAP_CHAIN_DESC sd = {
		.BufferDesc = {
			.Width = static_cast<UINT32>(WinSize.x),
			.Height = static_cast<UINT32>(WinSize.y),
			.RefreshRate = bestMode.RefreshRate,  
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED
		},
		.SampleDesc = {
			.Count = 1,
			.Quality = 0
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = SWAP_CHAIN_BUFFER_COUNT,
		.OutputWindow = hwnd,
		.Windowed = false,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	};

	hr = dxgi->CreateSwapChain(cmdQueue.Get(), &sd, &tempSwapChain);
	MASSERT(SUCCEEDED(hr), "Failed to create SwapChain");

	hr = tempSwapChain.As(&swapChain);
	MASSERT(SUCCEEDED(hr), "Failed to cast to IDXGISwapChain4");

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));

	backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	OutputDebugStringA("=== SwapChain Creation Complete ===\n");
}

void DX12Core::CreateRenderTargetView()
{
	int _rtvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvHeapSize);
		device->CreateRenderTargetView(rtvBuffer[i].Get(), nullptr, rtvHandle[i]);
	}
}

void DX12Core::CreateDepthStencilBuffer(DXGI_FORMAT dsvformat)
{
	dsvFormat = dsvformat;

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(dsvFormat, WinSize.x, WinSize.y);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimizedClearValue = CD3DX12_CLEAR_VALUE(dsvFormat, 1.0f, 0);

	device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&dsvBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap));

	dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(dsvBuffer.Get(), nullptr, dsvHandle);
}

void DX12Core::CreateGBuffer()
{
	OutputDebugStringA("Create G Buffer\n");

	// === 1. G-Buffer 텍스처들 생성 ===
	D3D12_RESOURCE_DESC rtDesc = {};
	rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rtDesc.Width = WinSize.x;
	rtDesc.Height = WinSize.y;
	rtDesc.DepthOrArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.SampleDesc.Count = 1;
	rtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};

	// RT0: BaseColor.rgb + Metallic.r (RGBA8UN)
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = rtDesc.Format;
	memset(clearValue.Color, 0, sizeof(clearValue.Color));

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[0]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[0]");

	// RT1: Normal.xyz + Roughness.r (RGBA32F)  
	rtDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[1]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[1]");

	// RT2: WorldPos.xyz + A0.r (RGBA32F)
	rtDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[2]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[2]");

	// RT3: Emossion.rgb + Alpha.r (RGBA32F)
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[3]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[3]");

	// === 2. RTV Descriptor Heap 생성 ===
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 4;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&gBufferRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RTV Heap");

	// === 3. SRV Descriptor Heap 생성 (라이팅 패스에서 읽기용) ===
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&gBufferSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer SRV Heap");

	// === 4. RTV들 생성 ===
	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gBufferRTVHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 4; ++i) {
		gBufferRTVHandles[i] = rtvHandle;
		device->CreateRenderTargetView(gBufferRT[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvSize;

		OutputDebugStringA(("G-Buffer RT" + to_string(i) + " RTV created\n").c_str());
	}

	// === 5. SRV들 생성 ===
	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = gBufferSRVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = gBufferSRVHeap->GetGPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 4; ++i) {
		gBufferSRVHandles[i] = srvGpuHandle;
		device->CreateShaderResourceView(gBufferRT[i].Get(), nullptr, srvCpuHandle);

		srvCpuHandle.ptr += srvSize;
		srvGpuHandle.ptr += srvSize;

		OutputDebugStringA(("G-Buffer RT" + to_string(i) + " SRV created\n").c_str());
	}

	OutputDebugStringA("G-Buffer created successfully!\n");
}

void DX12Core::CreateShadowMap()
{
	D3D12_RESOURCE_DESC shadowDesc = {};
	shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	shadowDesc.Width = WinSize.x;
	shadowDesc.Height = WinSize.y;
	shadowDesc.DepthOrArraySize = 1;
	shadowDesc.MipLevels = 1;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &shadowDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		IID_PPV_ARGS(&shadowMapTexture));
	MASSERT(SUCCEEDED(hr), "Failed to create shadowMap Texture!!\n");

	D3D12_DESCRIPTOR_HEAP_DESC shadowDSVDesc = {};
	shadowDSVDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	shadowDSVDesc.NumDescriptors = 1;
	shadowDSVDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&shadowDSVDesc, IID_PPV_ARGS(&shadowMapDSVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create shadowMap DSV Heap!!\n");

	D3D12_DESCRIPTOR_HEAP_DESC shadowRTVDesc = {};
	shadowRTVDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	shadowRTVDesc.NumDescriptors = 1;
	shadowRTVDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device->CreateDescriptorHeap(&shadowRTVDesc, IID_PPV_ARGS(&shadowMapSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create shadowMap RTV Heap!!\n");

	shadowMapDSVHandle = shadowMapDSVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(shadowMapTexture.Get(), &dsvDesc, shadowMapDSVHandle);

	shadowMapSRVHandle = shadowMapSRVHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = shadowMapSRVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(shadowMapTexture.Get(), &srvDesc, srvCpuHandle);

	OutputDebugStringA("Shadow Map creation succeed!!\n");
}

void DX12Core::BeginShadowPass()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMapTexture.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->OMSetRenderTargets(0, nullptr, FALSE, &shadowMapDSVHandle);

	cmdList->ClearDepthStencilView(shadowMapDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT shadowViewport = {};
	shadowViewport.Width = static_cast<float>(SHADOW_MAP_SIZE);
	shadowViewport.Height = static_cast<float>(SHADOW_MAP_SIZE);
	shadowViewport.MinDepth = 0.0f;
	shadowViewport.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &shadowViewport);

	D3D12_RECT shadowRect = { 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
	cmdList->RSSetScissorRects(1, &shadowRect);

	//OutputDebugStringA("Shadow Pass started!!\n");
}

void DX12Core::EndShadowPass()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMapTexture.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);

	D3D12_VIEWPORT mainViewPort = {};
	mainViewPort.Width = static_cast<float>(WinSize.x);
	mainViewPort.Height = static_cast<float>(WinSize.y);
	mainViewPort.MinDepth = 0.0f;
	mainViewPort.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &mainViewPort);

	D3D12_RECT mainRect = { 0, 0, WinSize.x, WinSize.y };
	cmdList->RSSetScissorRects(1, &mainRect);

	//OutputDebugStringA("Shadow Pass ended!!\n");
}

void DX12Core::BeginForwardPass()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHandle[backBufferIndex];
	cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsvHandle);

	cmdList->SetGraphicsRootSignature(GetRootSig()->Get());

	ForwardLightConstants light = { {0, 0, -1}, 0, {1, 1, 1}, 0.6f };
	GetForwardLightCB()->CopyData(&light, sizeof(ForwardLightConstants));
	cmdList->SetGraphicsRootConstantBufferView(4, GetForwardLightCB()->GetGPUVirtualAddress());		// 레지 넘버링 부분

	//OutputDebugStringA("Forward pass started\n");
}

void DX12Core::BeginGBufferPass()
{
	// 첫 번째 프레임에서는 상태 전환 건너뛰기
	static bool firstFrame = true;

	if (!firstFrame) {
		// 기존 상태 전환 코드
		D3D12_RESOURCE_BARRIER barriers[4];
		for (int i = 0; i < 4; ++i) {
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
				gBufferRT[i].Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
		}
		cmdList->ResourceBarrier(4, barriers);
	}
	else {
		firstFrame = false;
	}

	// G-Buffer 4개를 렌더 타겟으로 설정
	cmdList->OMSetRenderTargets(4, gBufferRTVHandles, FALSE, &dsvHandle);

	// G-Buffer 클리어 (검은색으로)
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (int i = 0; i < 4; ++i) {
		cmdList->ClearRenderTargetView(gBufferRTVHandles[i], clearColor, 0, nullptr);
	}

	// Depth 클리어
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//OutputDebugStringA("G-Buffer Pass started\n");
}

void DX12Core::EndGBufferPass()
{
	// G-Buffer를 RTV → SRV로 상태 변경 (라이팅 패스에서 읽기 위해)
	D3D12_RESOURCE_BARRIER barriers[4];
	for (int i = 0; i < 4; ++i) {
		barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
			gBufferRT[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	}
	cmdList->ResourceBarrier(4, barriers);

	//OutputDebugStringA("G-Buffer Pass ended\n");
}

void DX12Core::BeginLightingPass()
{
	SetupLightng();

	// 백버퍼를 렌더 타겟으로 설정
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHandle[backBufferIndex];
	cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);  // Depth 사용 안함

	cmdList->SetGraphicsRootSignature(GetRootSig()->Get());

	// G-Buffer SRV Heap을 셰이더에 바인딩
	ID3D12DescriptorHeap* heaps[] = { gBufferSRVHeap.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);

	// G-Buffer SRV 테이블 바인딩 (root parameter 13번)
	cmdList->SetGraphicsRootDescriptorTable(13, gBufferSRVHeap->GetGPUDescriptorHandleForHeapStart());		// 레지 넘버링 부분

	//OutputDebugStringA("Lighting Pass started\n");
}

void DX12Core::SetupLightng()
{
	// 50개 조명 설정 (Directional 2개 + Point Light 48개)
	static bool lightsInitialized = false;
	static DeferredLightConstants lightData = {};
	if (!lightsInitialized) {
		lightData.lightCount = 25;

		// 기존 directional light 유지
		lightData.lights[0] = {
			{0, 0, -1}, 0,               // direction
			{1, 1, 1}, 0.2f,             // color, intensity
			0,                           // type: directional
			{0, 0, 0}                    // padding
		};
		lightData.lights[1] = {
			{0, 0, 1}, 0,               // direction
			{1, 1, 1}, 0.25f,             // color, intensity
			0,                           // type: directional
			{0, 0, 0}                    // padding
		};

		lightData.lights[2] = {
			{-27.f, 29.f, -70.0f}, 2000.0f,                // direction
			{0.074, 0, 1}, 0.15f,        // color, intensity
			1,                           // type: directional
			{0, 0, 0}                    // padding
		};

		lightData.lights[3] = {
			{27.f, 29.f, -70.0f}, 2000.0f,                // direction
			{0.074, 0, 1}, 0.15f,        // color, intensity
			1,                           // type: directional
			{0, 0, 0}                    // padding
		};

		lightData.lights[4] = {
			{0, 0, -1}, 0,                // direction
			{1, 1, 1}, 0.2f,        // color, intensity
			0,                           // type: directional
			{0, 0, 0}                    // padding
		};

		// Point lights 48개 - 두 줄로 24개씩 배치
		float spacing = 15.0f;
		float height = 4.0;           // 높이 2.5
		float leftX = -7.0f;           // 왼쪽 줄 X 위치
		float rightX = 7.0f;           // 오른쪽 줄 X 위치

		for (int i = 5; i < 25; ++i) {
			int lightIndex = i - 5;   // 0~47 인덱스
			int rowIndex = lightIndex % 10;  // 0~23 (각 줄의 인덱스)
			bool isLeftRow = (lightIndex < 10);  // 첫 24개는 왼쪽 줄

			float x = isLeftRow ? leftX : rightX;
			float z = -(rowIndex * spacing);  // 0, -2, -4, -6, ... -46

			// 색상: 왼쪽 줄은 파란색, 오른쪽 줄은 빨간색
			XMFLOAT3 color = isLeftRow ?
				XMFLOAT3{ 1.0f, 0.25f, 0.0f } :  // 파란색 (왼쪽 줄)
				XMFLOAT3{ 1.0f, 0.25f, 0.0f };   // 빨간색 (오른쪽 줄)

			lightData.lights[i] = {
				{x, height, z + 70.0f}, 10.0f,    // position, range
				color, 1.0f,             // color, intensity
				1,                       // type: point light
				{0, 0, 0}               // padding
			};
		}

		lightsInitialized = true;
	}
	deferredLightCB->CopyData(&lightData, sizeof(DeferredLightConstants));
}

void DX12Core::RenderFullscreenQuad()
{
	auto cmdList = GetGraphicsCmdList();

	// 라이팅 PSO 설정
	cmdList->SetPipelineState(shader->GetLightingPSO());
	cmdList->SetGraphicsRootSignature(GetRootSig()->Get());

	// 라이트 데이터 바인딩
	cmdList->SetGraphicsRootConstantBufferView(3, GetDeferredLightCB()->GetGPUVirtualAddress());		// 레지 넘버링 부분

	// 정점 버퍼 없이 6개 정점으로 사각형 그리기 (2개 삼각형)
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);  // 6개 정점

	//OutputDebugStringA("Fullscreen quad rendered\n");
}

void DX12Core::RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvBuffer[backBufferIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	cmdList->ResourceBarrier(1, &barrier);

	cmdList->RSSetViewports(1, &vp);
	cmdList->RSSetScissorRects(1, &rect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHandle[backBufferIndex];

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHandle;

	cmdList->ClearRenderTargetView(rtv, backgroundColor, 0, nullptr);
	cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void DX12Core::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvBuffer[backBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	cmdList->ResourceBarrier(1, &barrier);
	cmdList->Close();

	ID3D12CommandList* cmdListArr[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	HRESULT hr = swapChain->Present(0, 0);

	WaitSync();

	// ResizeBuffers가 필요한 경우 처리
	if (hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_STATUS_OCCLUDED) {
		// 백버퍼 참조 해제
		for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
			rtvBuffer[i].Reset();
		}

		// ResizeBuffers 호출
		swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

		// 백버퍼 다시 가져오기
		for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
			swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));
		}

		// 렌더 타겟 뷰 다시 생성
		CreateRenderTargetView();

		// 인덱스 갱신
		backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	}
	else if (SUCCEEDED(hr)) {
		backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	}
}

void DX12Core::WaitSync()
{
	fenceValue++;

	cmdQueue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void DX12Core::FlushCommandQueue()
{
	cmdList->Close();

	ID3D12CommandList* lists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(1, lists);

	WaitSync();
}

void DX12Core::ResetCommandQueue()
{
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc.Get(), nullptr);
}

ID3D12Device* DX12Core::GetDevice() const
{
	return device.Get();
}

ID3D12CommandQueue* DX12Core::GetCmdQueue() const
{
	return cmdQueue.Get();
}

ID3D12GraphicsCommandList* DX12Core::GetGraphicsCmdList() const
{
	return cmdList.Get();
}

IDXGISwapChain4* DX12Core::GetSwapChain() const
{
	return swapChain.Get();
}

RootSignature* DX12Core::GetRootSig() const
{
	return rootSig.get();
}

Shader* DX12Core::GetShader() const
{
	return shader.get();
}

UploadBuffer* DX12Core::GetFrameCB() const
{
	return frameCB.get();
}

UploadBuffer* DX12Core::GetSceneCB() const
{
	return sceneCB.get();
}

UploadBuffer* DX12Core::GetDeferredLightCB() const
{
	return deferredLightCB.get();
}

UploadBuffer* DX12Core::GetForwardLightCB() const
{
	return forwardLightCB.get();
}

void DX12Core::SetBackgroundColor(const float* color)
{
	backgroundColor = color;
}