#include "pch.h"
#include "Engine.h"

void Engine::Init(const WindowInfo& info)
{
	_window = info;
	ResizeWindow(info.width, info.height);

	_viewPort = { 0, 0, static_cast<FLOAT>(info.width), static_cast<FLOAT>(info.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, info.width, info.height);

	_device = make_shared<Device>();
	_cmdQueue = make_shared<CommandQueue>();
	_swapChain = make_shared<SwapChain>();
	_rootSignature = make_shared<RootSignature>();
	_mesh = make_shared<Mesh>();
	_shader = make_shared<Shader>();
	_constantBuffer = make_shared<ConstantBuffer>();

	_device->Init();
	_cmdQueue->Init(_device->GetDevice(), _swapChain);
	_swapChain->Init(info, _device->GetDevice(), _device->GetDXGI(), _cmdQueue->GetCmdQueue());
	_rootSignature->Init(_device->GetDevice());
	
	_mesh->Init();

	_shader->Init(L"..\\Resources\\Shader\\default.hlsli");

	_constantBuffer->Init(sizeof(Transform), 256);

	_cmdQueue->WaitSync();
}

void Engine::Render()
{
	RenderBegin();

	// TODO
	_shader->Update();

	/*{
		Transform t;
		t.offset = XMFLOAT3(-0.7f, 0.0f, 0.0f);
		_mesh->SetTransform(t);

		_mesh->Render();
	}

	{
		Transform t;
		t.offset = XMFLOAT3(0.7f, 0.0f, 0.0f);
		_mesh->SetTransform(t);

		_mesh->Render();
	}

	{
		Transform t;
		t.offset = XMFLOAT3(0.0f, 0.7f, 0.0f);
		_mesh->SetTransform(t);

		_mesh->Render();
	}

	{
		Transform t;
		t.offset = XMFLOAT3(0.0f, -0.7f, 0.0f);
		_mesh->SetTransform(t);

		_mesh->Render();
	}*/

	for (int i = 0; i < 9; ++i)
	{
		for (int j = 0; j < 9; ++j)
		{
			Transform t;
			t.offset = XMFLOAT3(-0.8f + (0.2f * i), -0.8f + (0.2f * j), 0.0f);
			_mesh->SetTransform(t);

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
}
