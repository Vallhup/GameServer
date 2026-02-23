#pragma once

class DeviceContext
{
public:
	void Initialize(HWND hwnd);

	void WaitSync();
	void FlushCommandQueue();
	void ResetCommandQueue();

	ID3D12Device* GetDevice() const;
	IDXGIFactory7* GetDxgi() const;
	ID3D12CommandQueue* GetCmdQueue() const;
	ID3D12CommandAllocator* GetCmdAlloc() const;
	ID3D12GraphicsCommandList* GetGraphicsCmdList() const;
	ID3D12GraphicsCommandList* GetActiveCmdList() const;

	void SetLoadingMode(bool loading);
	void ExecuteLoadingCommands();

private:
	void CreateDXGI(HWND hwnd);
	void CreateDevice();
	void CreateCommandObjects();

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
};

