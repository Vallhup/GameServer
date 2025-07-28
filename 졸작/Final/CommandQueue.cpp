#include "pch.h"
#include "CommandQueue.h"
#include "SwapChain.h"

CommandQueue::~CommandQueue()
{
	CloseHandle(fenceevent);
}

void CommandQueue::Initialize(ID3D12Device* device, shared_ptr<SwapChain> swapChain)
{
	CreateCommandQueue(device);
	CreateCommandAlloc(device);
	CreateCommandList(device);

	CreateFence(device);
	fenceevent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CommandQueue::RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, shared_ptr<SwapChain> swapchain, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	cmdalloc->Reset();
	cmdlist->Reset(cmdalloc.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapchain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	cmdlist->ResourceBarrier(1, &barrier);

	cmdlist->RSSetViewports(1, &vp);
	cmdlist->RSSetScissorRects(1, &rect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapchain->GetBackRTV();

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHandle;

	cmdlist->ClearRenderTargetView(rtv, backgroundcolor, 0, nullptr);
	cmdlist->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdlist->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void CommandQueue::RenderEnd(shared_ptr<SwapChain> swapchain)
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapchain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,		
		D3D12_RESOURCE_STATE_PRESENT);			

	cmdlist->ResourceBarrier(1, &barrier);
	cmdlist->Close();

	ID3D12CommandList* cmdListArr[] = { cmdlist.Get() };
	cmdqueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	swapchain->Present();		

	WaitSync();					

	swapchain->SwapIndex();	
}

void CommandQueue::WaitSync()
{
	fencevalue++;

	cmdqueue->Signal(fence.Get(), fencevalue);

	if (fence->GetCompletedValue() < fencevalue)
	{
		fence->SetEventOnCompletion(fencevalue, fenceevent);
		WaitForSingleObject(fenceevent, INFINITE);
	}
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCmdQueue() const
{
	return cmdqueue;
}

ComPtr<ID3D12CommandAllocator> CommandQueue::GetCmdAlloc() const
{
	return cmdalloc;
}

ComPtr<ID3D12GraphicsCommandList> CommandQueue::GetCmdList() const
{
	return cmdlist;
}

void CommandQueue::CreateCommandQueue(ID3D12Device* device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdqueue));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command Queue\n");
}

void CommandQueue::CreateCommandAlloc(ID3D12Device* device)
{
	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create Command Allocator\n");
}

void CommandQueue::CreateCommandList(ID3D12Device* device)
{
	HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), nullptr, IID_PPV_ARGS(&cmdlist));

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
	backgroundcolor = color;
}
