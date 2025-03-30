#pragma once

class CommandQueue
{
public:
	~CommandQueue();

	void Init(ComPtr<ID3D12Device> device, shared_ptr<class SwapChain> swapChain);

	void RenderBegin(const D3D12_VIEWPORT* vp, const D3D12_RECT* rect);
	void RenderEnd();

	void WaitSync();

	ComPtr<ID3D12CommandQueue> GetCmdQueue() { return _cmdQueue; }

private:
	ComPtr<ID3D12GraphicsCommandList>	_cmdList;
	ComPtr<ID3D12CommandAllocator>		_cmdAlloc;
	ComPtr<ID3D12CommandQueue>			_cmdQueue;

	ComPtr<ID3D12Fence>		_fence;
	uint64					_fenceValue = 0;
	HANDLE					_fenceEvent = INVALID_HANDLE_VALUE;

	shared_ptr<class SwapChain>			_swapChain;
};