#include "pch.h"
#include "DX12Core.h"
#include "RootSignature.h"
#include "Shader.h"
#include "Timer.h"
#include "LightManager.h"

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
	shadowFrameCB = make_unique<UploadBuffer>();
	fogCB = make_unique<UploadBuffer>();

	lightMgr = make_unique<LightManager>();

	rootSig->Initialize(GetDevice());
	shader->InitializeAllShaders(GetDevice(), GetRootSig()->Get());
	frameCB->Initialize(GetDevice(), sizeof(FrameConstants));
	sceneCB->Initialize(GetDevice(), 256 * 1000);
	shadowFrameCB->Initialize(GetDevice(), sizeof(XMMATRIX) * 2);
	fogCB->Initialize(GetDevice(), sizeof(FogConstants));

	lightMgr->Initialize(GetDevice());

	CreateDepthStencilBuffer();
	CreateShadowMap();
	CreateGBuffer();
	CreateDeferredRenderingDescriptors();
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

	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&loadingCmdAlloc));
	MASSERT(SUCCEEDED(hr), "Failed to create Loading Command Allocator");
	
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, loadingCmdAlloc.Get(), nullptr, IID_PPV_ARGS(&loadingCmdList));
	MASSERT(SUCCEEDED(hr), "Failed to create Loading Command List");

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

	TIMER.SetTargetFPS(bestRefreshRate);

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
		.Windowed = true,
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

	// SRGB 뷰로 생성하여 하드웨어 감마 보정 적용
	D3D12_RENDER_TARGET_VIEW_DESC rtvViewDesc = {};
	rtvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvHeapSize);
		device->CreateRenderTargetView(rtvBuffer[i].Get(), &rtvViewDesc, rtvHandle[i]);
	}
}

void DX12Core::CreateDepthStencilBuffer(DXGI_FORMAT dsvformat)
{
	dsvFormat = dsvformat;

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, WinSize.x, WinSize.y);
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

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dsvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(dsvBuffer.Get(), &dsvDesc, dsvHandle);
}

void DX12Core::CreateGBuffer()
{
	OutputDebugStringA("Create G Buffer\n");

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

	// RT0: BaseColor.rgb + Metallic (RGBA8UN)
	rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Format = rtDesc.Format;
	memset(clearValue.Color, 0, sizeof(clearValue.Color));

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[0]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[0]");

	// RT1: Normal.xyz + Roughness (RGBA16F)  
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[1]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[1]");

	// RT2: Emission.rgb + A0 (RGBA16F)
	rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	clearValue.Format = rtDesc.Format;
	hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue,
		IID_PPV_ARGS(&gBufferRT[2]));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RT[2]");

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 3;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&gBufferRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create G-Buffer RTV Heap");

	UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gBufferRTVHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 3; ++i) {
		gBufferRTVHandles[i] = rtvHandle;
		device->CreateRenderTargetView(gBufferRT[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtvSize;

		OutputDebugStringA(("G-Buffer RT" + to_string(i) + " RTV created\n").c_str());
	}

	OutputDebugStringA("G-Buffer created successfully!\n");
}

void DX12Core::CreateShadowMap()
{
	D3D12_RESOURCE_DESC shadowDesc = {};
	shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	shadowDesc.Width = SHADOW_MAP_SIZE;
	shadowDesc.Height = SHADOW_MAP_SIZE;
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

	shadowMapDSVHandle = shadowMapDSVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(shadowMapTexture.Get(), &dsvDesc, shadowMapDSVHandle);

	OutputDebugStringA("Shadow Map creation succeed!!\n");
}

void DX12Core::CreateDeferredRenderingDescriptors()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 5; // Gbuffer(3) + depth(1) + shadow(1)
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&deferredSRVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create Deferred SRV Heap");

	UINT srvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = deferredSRVHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = deferredSRVHeap->GetGPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 3; ++i) {
		gBufferSRVHandles[i] = srvGpuHandle;
		device->CreateShaderResourceView(gBufferRT[i].Get(), nullptr, srvCpuHandle);

		srvCpuHandle.ptr += srvSize;
		srvGpuHandle.ptr += srvSize;

		OutputDebugStringA(("G-Buffer RT" + to_string(i) + " SRV created\n").c_str());
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
	depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrvDesc.Texture2D.MipLevels = 1;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(dsvBuffer.Get(), &depthSrvDesc, srvCpuHandle);
	srvCpuHandle.ptr += srvSize;
	srvGpuHandle.ptr += srvSize;

	shadowMapSRVHandle = srvGpuHandle;  
	D3D12_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
	shadowSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shadowSrvDesc.Texture2D.MipLevels = 1;
	shadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(shadowMapTexture.Get(), &shadowSrvDesc, srvCpuHandle);

	OutputDebugStringA("Shadow Map SRV Created\n");

	OutputDebugStringA("Deferred Rendering Descriptors created successfully!!\n");
}

void DX12Core::BeginShadowPass()
{
	XMVECTOR lightDir = XMVectorSet(0, 0, -1.f, 0); 
	XMVECTOR lightPos = XMVectorSet(80.0f + 45.0f, 40.0f, 80.0f + 60.0f, 1);
	XMVECTOR targetPos = XMVectorSet(80.0f, 0.0f, 80.0f, 1);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	XMMATRIX lightView = XMMatrixTranspose(XMMatrixLookAtLH(lightPos, targetPos, up));
	XMMATRIX lightProjection = XMMatrixTranspose(XMMatrixOrthographicLH(180.0f, 180.0f, 1.0f, 200.0f));
	
	shadowFrameCB->CopyData(&lightView, sizeof(XMMATRIX), 0);
	shadowFrameCB->CopyData(&lightProjection, sizeof(XMMATRIX), sizeof(XMMATRIX));

	static bool firstShadowPass = true;

	if (!firstShadowPass) {
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMapTexture.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
		cmdList->ResourceBarrier(1, &barrier);
	}
	else {
		firstShadowPass = false;
	}

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

	cmdList->SetGraphicsRootSignature(GetRootSig()->Get());
	cmdList->SetGraphicsRootConstantBufferView(5, shadowFrameCB->GetGPUVirtualAddress());		

	//OutputDebugStringA("Shadow Pass started!!\n");
}

void DX12Core::EndShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMapTexture.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);
	cmdList->RSSetViewports(1, &vp);
	cmdList->RSSetScissorRects(1, &rect);

	//OutputDebugStringA("Shadow Pass ended!!\n");
}

void DX12Core::BeginForwardPass()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		dsvBuffer.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	cmdList->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHandle[backBufferIndex];
	cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsvHandle);

	cmdList->SetGraphicsRootSignature(GetRootSig()->Get());
	cmdList->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(4, lightMgr->GetForwardLightCB()->GetGPUVirtualAddress());		

	/*FogConstants fog = { { 0.5f, 0.5f, 0.5f, 1.0f }, 2.0f, 3.5f, 0.0f, 20.0f, 6.0f, {0, 0, 0} };
	GetFogCB()->CopyData(&fog, sizeof(FogConstants));
	cmdList->SetGraphicsRootConstantBufferView(14, GetFogCB()->GetGPUVirtualAddress());*/

	//OutputDebugStringA("Forward pass started\n");
}

void DX12Core::BeginGBufferPass()
{
	static bool firstFrame = true;

	if (!firstFrame) {
		D3D12_RESOURCE_BARRIER barriers[3];
		for (int i = 0; i < 3; ++i) {
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
				gBufferRT[i].Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
		}
		cmdList->ResourceBarrier(3, barriers);
	}
	else {
		firstFrame = false;
	}

	cmdList->OMSetRenderTargets(3, gBufferRTVHandles, FALSE, &dsvHandle);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (int i = 0; i < 3; ++i) {
		cmdList->ClearRenderTargetView(gBufferRTVHandles[i], clearColor, 0, nullptr);
	}

	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DX12Core::EndGBufferPass()
{
	D3D12_RESOURCE_BARRIER barriers[4];
	for (int i = 0; i < 3; ++i) {
		barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
			gBufferRT[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	}
	barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(
		dsvBuffer.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	cmdList->ResourceBarrier(4, barriers);
}

void DX12Core::BeginLightingPass()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHandle[backBufferIndex];
	cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);  

	cmdList->SetGraphicsRootSignature(GetRootSig()->Get());

	cmdList->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(3, lightMgr->GetDeferredLightCB()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(5, shadowFrameCB->GetGPUVirtualAddress());					

	ID3D12DescriptorHeap* heaps[] = { deferredSRVHeap.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);

	cmdList->SetGraphicsRootDescriptorTable(13, deferredSRVHeap->GetGPUDescriptorHandleForHeapStart());		

	/*FogConstants fog = { { 0.5f, 0.5f, 0.5f, 1.0f }, 2.0f, 3.5f, 0.0f, 20.0f, 6.0f, {0, 0, 0} };
	GetFogCB()->CopyData(&fog, sizeof(FogConstants));
	cmdList->SetGraphicsRootConstantBufferView(14, GetFogCB()->GetGPUVirtualAddress());*/

	//OutputDebugStringA("Lighting Pass started\n");
}

void DX12Core::RenderFullscreenQuad()
{
	cmdList->SetPipelineState(shader->GetPSO(PSOType::Lighting));

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);  

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

	if (hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_STATUS_OCCLUDED) {
		for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
			rtvBuffer[i].Reset();
		}

		swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

		for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
			swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));
		}

		CreateRenderTargetView();

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

ID3D12GraphicsCommandList* DX12Core::GetLoadingCmdList() const
{
	return loadingCmdList.Get();
}

ID3D12GraphicsCommandList* DX12Core::GetActiveCmdList() const
{
	return isLoadingMode ? loadingCmdList.Get() : cmdList.Get();
}

void DX12Core::SetLoadingMode(bool loading)
{
	isLoadingMode = loading;
}

void DX12Core::ExecuteLoadingCommands()
{
	loadingCmdList->Close();
	ID3D12CommandList * lists[] = { loadingCmdList.Get() };
	cmdQueue->ExecuteCommandLists(1, lists);
	WaitSync();
	loadingCmdAlloc->Reset();
	loadingCmdList->Reset(loadingCmdAlloc.Get(), nullptr);
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

UploadBuffer* DX12Core::GetFogCB() const
{
	return fogCB.get();
}

void DX12Core::SetBackgroundColor(const float* color)
{
	backgroundColor = color;
}

void DX12Core::SetPlayerPosForShadow(const XMFLOAT3& pos)
{
	playerCurrentPos = pos;
}

ID3D12DescriptorHeap* DX12Core::GetDeferredSRVHeap() const
{
	return deferredSRVHeap.Get();
}
