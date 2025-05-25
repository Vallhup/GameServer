#pragma once

class SwapChain;

class CommandQueue
{
public:
	~CommandQueue();

	void Initialize(ID3D12Device* device, shared_ptr<SwapChain> swapChain);

	void RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, shared_ptr<SwapChain> swapchain, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
	void RenderEnd(shared_ptr<SwapChain> swapchain);

	void WaitSync();

	ComPtr<ID3D12CommandQueue> GetCmdQueue() const;
	ComPtr<ID3D12CommandAllocator> GetCmdAlloc() const;
	ComPtr<ID3D12GraphicsCommandList> GetCmdList() const;

private:
	void CreateCommandQueue(ID3D12Device* device);
	void CreateCommandAlloc(ID3D12Device* device);
	void CreateCommandList(ID3D12Device* device);
	void CreateFence(ID3D12Device* device);

private:
	ComPtr<ID3D12CommandQueue>			cmdqueue;
	ComPtr<ID3D12CommandAllocator>		cmdalloc;
	ComPtr<ID3D12GraphicsCommandList>	cmdlist;

	ComPtr<ID3D12Fence>		fence;
	UINT64					fencevalue = 0;
	HANDLE					fenceevent = INVALID_HANDLE_VALUE;

	static constexpr float backgroundcolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

