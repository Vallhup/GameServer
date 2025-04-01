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
	_tableDescHeap = make_shared<TableDescriptorHeap>();
	_texture = make_shared<Texture>();

	_device->Init();
	_cmdQueue->Init(_device->GetDevice(), _swapChain);
	_swapChain->Init(info, _device->GetDevice(), _device->GetDXGI(), _cmdQueue->GetCmdQueue());
	_rootSignature->Init();
	_mesh->Init();
	_shader->Init(L"..\\Resources\\Shader\\default.hlsli");
	_constantBuffer->Init(sizeof(Transform), 256);
	_tableDescHeap->Init(256);
	_texture->Init(L"..\\Resources\\Texture\\FennecFox.jpg");

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

	// 아래처럼 81개 그릴거면, 81 * 5개의 CBV를 만들어야함 (총 405개) -> _constantBuffer->Init(sizeof(Transform), 512);로 수정
	
	Transform t;
	t.offset = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	_mesh->SetTransform(t);

	_mesh->SetTexture(_texture);

	_mesh->Render();
	

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
