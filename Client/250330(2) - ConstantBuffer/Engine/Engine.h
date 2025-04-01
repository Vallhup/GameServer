#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "Mesh.h"
#include "Shader.h"
#include "ConstantBuffer.h"

class Engine
{
public:
	void Init(const WindowInfo& info);
	void Render();

	void RenderBegin();
	void RenderEnd();

	void ResizeWindow(int32 width, int32 height);

	shared_ptr<Device>			GetDevice() { return _device; }
	shared_ptr<CommandQueue>	GetCmdQueue() { return _cmdQueue; }
	shared_ptr<SwapChain>		GetSwapChain() { return	_swapChain; }
	shared_ptr<RootSignature>	GetRootSignature() { return _rootSignature; }
	shared_ptr<ConstantBuffer>	GetConstantBuffer() { return _constantBuffer; }

private:
	WindowInfo		_window;
	D3D12_VIEWPORT	_viewPort = {};
	D3D12_RECT		_scissorRect = {};

	shared_ptr<Device>				_device;
	shared_ptr<CommandQueue>		_cmdQueue;
	shared_ptr<SwapChain>			_swapChain;
	shared_ptr<RootSignature>		_rootSignature;
	shared_ptr<Mesh>				_mesh;
	shared_ptr<Shader>				_shader;
	shared_ptr<ConstantBuffer>		_constantBuffer;
};