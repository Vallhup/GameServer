#include "pch.h"
#include "CommandQueue.h"
#include "SwapChain.h"

CommandQueue::~CommandQueue()
{
	CloseHandle(fenceEvent);
}

void CommandQueue::Initialize(ID3D12Device* device)
{
	CreateCommandQueue(device);
	CreateCommandAlloc(device);
	CreateCommandList(device);

	CreateFence(device);
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CommandQueue::RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, shared_ptr<SwapChain> swapchain, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapchain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	cmdList->ResourceBarrier(1, &barrier);

	cmdList->RSSetViewports(1, &vp);
	cmdList->RSSetScissorRects(1, &rect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapchain->GetBackRTV();

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHandle;

	cmdList->ClearRenderTargetView(rtv, backgroundColor, 0, nullptr);
	cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void CommandQueue::RenderEnd(shared_ptr<SwapChain> swapchain)
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapchain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,		
		D3D12_RESOURCE_STATE_PRESENT);			

	cmdList->ResourceBarrier(1, &barrier);
	cmdList->Close();

	ID3D12CommandList* cmdListArr[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	swapchain->Present();		

	WaitSync();					

	swapchain->SwapIndex();	
}

void CommandQueue::WaitSync()
{
	fenceValue++;

	cmdQueue->Signal(fence.Get(), fenceValue);

	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCmdQueue() const
{
	return cmdQueue;
}

ComPtr<ID3D12CommandAllocator> CommandQueue::GetCmdAlloc() const
{
	return cmdAlloc;
}

ComPtr<ID3D12GraphicsCommandList> CommandQueue::GetCmdList() const
{
	return cmdList;
}

void CommandQueue::CreateCommandQueue(ID3D12Device* device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command Queue\n");
}

void CommandQueue::CreateCommandAlloc(ID3D12Device* device)
{
	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command Allocator\n");
}

void CommandQueue::CreateCommandList(ID3D12Device* device)
{
	HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command List\n");
}

void CommandQueue::CreateFence(ID3D12Device* device)
{
	HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Fence\n");
}

void CommandQueue::SetBackgroundColor(const float* color)
{
	backgroundColor = color;
}
