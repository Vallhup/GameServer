#include "pch.h"
#include "DeviceContext.h"

void DeviceContext::Initialize(HWND hwnd)
{
	CreateDXGI(hwnd);
	CreateDevice();
	CreateCommandObjects();
}

void DeviceContext::WaitSync()
{
	fenceValue++;

	cmdQueue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void DeviceContext::FlushCommandQueue()
{
	cmdList->Close();

	ID3D12CommandList* lists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(1, lists);

	WaitSync();
}

void DeviceContext::ResetCommandQueue()
{
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc.Get(), nullptr);
}

ID3D12Device* DeviceContext::GetDevice() const
{
	return device.Get();
}

IDXGIFactory7* DeviceContext::GetDxgi() const
{
	return dxgi.Get();
}

ID3D12CommandQueue* DeviceContext::GetCmdQueue() const
{
	return cmdQueue.Get();
}

ID3D12CommandAllocator* DeviceContext::GetCmdAlloc() const
{
	return cmdAlloc.Get();
}

ID3D12GraphicsCommandList* DeviceContext::GetGraphicsCmdList() const
{
	return cmdList.Get();
}

ID3D12GraphicsCommandList* DeviceContext::GetActiveCmdList() const
{
	return isLoadingMode ? loadingCmdList.Get() : cmdList.Get();
}

void DeviceContext::SetLoadingMode(bool loading)
{
	isLoadingMode = loading;
}

void DeviceContext::ExecuteLoadingCommands()
{
	loadingCmdList->Close();
	ID3D12CommandList* lists[] = { loadingCmdList.Get() };
	cmdQueue->ExecuteCommandLists(1, lists);
	WaitSync();
	loadingCmdAlloc->Reset();
	loadingCmdList->Reset(loadingCmdAlloc.Get(), nullptr);
}

void DeviceContext::CreateDXGI(HWND hwnd)
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

void DeviceContext::CreateDevice()
{
	HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
	MASSERT(SUCCEEDED(hr), "Failed to create D3D12 device");
}

void DeviceContext::CreateCommandObjects()
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