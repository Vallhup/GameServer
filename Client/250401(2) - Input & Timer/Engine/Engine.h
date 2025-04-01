#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "SwapChain.h"
#include "RootSignature.h"
#include "Mesh.h"
#include "Shader.h"
#include "ConstantBuffer.h"
#include "TableDescriptorHeap.h"
#include "Texture.h"
#include "DepthStencilBuffer.h"

#include "Input.h"

class Engine
{
public:
	void Init(const WindowInfo& info);
	void Render();

	void Update();

	void RenderBegin();
	void RenderEnd();

	void ResizeWindow(int32 width, int32 height);

	shared_ptr<Device>				GetDevice() { return _device; }
	shared_ptr<CommandQueue>		GetCmdQueue() { return _cmdQueue; }
	shared_ptr<SwapChain>			GetSwapChain() { return	_swapChain; }
	shared_ptr<RootSignature>		GetRootSignature() { return _rootSignature; }
	shared_ptr<ConstantBuffer>		GetConstantBuffer() { return _constantBuffer; }
	shared_ptr<TableDescriptorHeap> GetTableDescHeap() { return _tableDescHeap; }
	shared_ptr<DepthStencilBuffer>	GetDepthStencilBuffer() { return _depthStencilBuffer; }

	shared_ptr<Input>				GetInput() { return _input; }

private:
	WindowInfo		_window;
	D3D12_VIEWPORT	_viewPort = {};
	D3D12_RECT		_scissorRect = {};

	// shared_ptr을 헤더에서 초기화 하기 위해선 전방선언이 아닌 헤더파일 include 필요
	shared_ptr<Device>				_device = make_shared<Device>();
	shared_ptr<CommandQueue>		_cmdQueue = make_shared<CommandQueue>();
	shared_ptr<SwapChain>			_swapChain = make_shared<SwapChain>();
	shared_ptr<RootSignature>		_rootSignature = make_shared<RootSignature>();
	shared_ptr<ConstantBuffer>		_constantBuffer = make_shared<ConstantBuffer>();
	shared_ptr<TableDescriptorHeap>	_tableDescHeap = make_shared<TableDescriptorHeap>();
	shared_ptr<DepthStencilBuffer>	_depthStencilBuffer = make_shared<DepthStencilBuffer>();

	// 아래 네개는 내가 실험하기 위해 추가한 것
	shared_ptr<Input>				_input = make_shared<Input>();

	shared_ptr<Mesh>				_mesh = make_shared<Mesh>();
	shared_ptr<Shader>				_shader = make_shared<Shader>();
	shared_ptr<Texture>				_texture = make_shared<Texture>();
};