#include "pch.h"
#include "Engine.h"

void Engine::Init(const WindowInfo& info)
{
	_window = info;

	_viewPort = { 0, 0, static_cast<FLOAT>(info.width), static_cast<FLOAT>(info.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, info.width, info.height);

	_device = make_shared<Device>();
	_cmdQueue = make_shared<CommandQueue>();
	_swapChain = make_shared<SwapChain>();
	_rootSignature = make_shared<RootSignature>();
	_constantBuffer = make_shared<ConstantBuffer>();
	_tableDescHeap = make_shared<TableDescriptorHeap>();
	_depthStencilBuffer = make_shared<DepthStencilBuffer>();

	_mesh = make_shared<Mesh>();
	_shader = make_shared<Shader>();
	_texture = make_shared<Texture>();

	_device->Init();
	_cmdQueue->Init(_device->GetDevice(), _swapChain);
	_swapChain->Init(info, _device->GetDevice(), _device->GetDXGI(), _cmdQueue->GetCmdQueue());
	_rootSignature->Init();
	_constantBuffer->Init(sizeof(Transform), 256);
	_tableDescHeap->Init(256);
	_depthStencilBuffer->Init(_window);

	_mesh->Init();
	_shader->Init(L"..\\Resources\\Shader\\default.hlsli");
	_texture->Init(L"..\\Resources\\Texture\\FennecFox.jpg");

	_cmdQueue->WaitSync();

	ResizeWindow(info.width, info.height);
}

void Engine::Render()
{
	RenderBegin();

	// TODO
	_shader->Update();

	for(int i = 0; i < 7; ++i)
	{
		for (int j = 0; j < 7; ++j)
		{
			Transform t;
			t.offset = XMFLOAT4(-0.75f + (i * 0.25), -0.75f + (j * 0.25), 0.01 * i, 0.0f);
			_mesh->SetTransform(t);

			_mesh->SetTexture(_texture);

			_mesh->Render();
		}
	}


	RenderEnd();
}

void Engine::RenderBegin()
{
	_cmdQueue->RenderBegin(&_viewPort, &_scissorRect);
}

void Engine::RenderEnd()
{
	_cmdQueue->RenderEnd();
}

void Engine::ResizeWindow(int32 width, int32 height)
{
	_window.width = width;
	_window.height = height;

	RECT rect = { 0, 0, _window.width, _window.height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	SetWindowPos(_window.hwnd, 0, 100, 100, width, height, 0);

	_depthStencilBuffer->Init(_window);
}
