#include "pch.h"
#include "DX12Core.h"
#include "RootSignature.h"
#include "Shader.h"

void DX12Core::Initialize(HWND hwnd)
{
	rootSig = make_unique<RootSignature>();
	shader = make_unique<Shader>();
	frameCB = make_unique<UploadBuffer>();
	sceneCB = make_unique<UploadBuffer>();
	animationCB = make_unique<UploadBuffer>();
	directionLightCB = make_unique<UploadBuffer>();

	CreateDXGI(hwnd);
	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain(hwnd);
	CreateRenderTargetView();
	rootSig->Initialize(GetDevice());
	shader->Initialize(GetDevice(), GetRootSig()->Get(), L"BasicVS.hlsli", L"BasicPS.hlsli");
	shader->InitializeComputeShader(GetDevice(), GetRootSig()->Get(), L"Animation.hlsli");
	frameCB->Initialize(GetDevice(), sizeof(XMMATRIX) * 2);
	sceneCB->Initialize(GetDevice(), 256 * 100);
	animationCB->Initialize(GetDevice(), sizeof(AnimationConstants));
	directionLightCB->Initialize(GetDevice(), sizeof(LightConstants));
	CreateDepthStencilBuffer();
}

void DX12Core::CreateDevice()
{
	HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create D3D12 device\n");
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

	if (FAILED(hr))
		OutputDebugStringA("Failed to create DXGI factory\n");
}

void DX12Core::CreateCommandObjects()
{
	// cmdQueue 持失
	D3D12_COMMAND_QUEUE_DESC desc = {
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command Queue\n");

	// cmdAlloc 持失
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command Allocator\n");

	// cmdList 持失
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command List\n");

	// fence 持失
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Fence\n");

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void DX12Core::CreateSwapChain(HWND hwnd)
{
	swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd = {
		.BufferDesc = {
			.Width = static_cast<UINT32>(WinSize.x),
			.Height = static_cast<UINT32>(WinSize.y),
			.RefreshRate = {
				.Numerator = 60,
				.Denominator = 1
				},
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

	dxgi->CreateSwapChain(cmdQueue.Get(), &sd, &swapChain);

	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffer[i]));
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

	swapChain->Present(0, 0);

	WaitSync();

	backBufferIndex = (backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
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

ID3D12GraphicsCommandList* DX12Core::GetGraphicsCmdList() const
{
	return cmdList.Get();
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

UploadBuffer* DX12Core::GetAnimationCB() const
{
	return animationCB.get();
}

UploadBuffer* DX12Core::GetDirectionalLightCB() const
{
	return directionLightCB.get();
}

void DX12Core::SetBackgroundColor(const float* color)
{
	backgroundColor = color;
}