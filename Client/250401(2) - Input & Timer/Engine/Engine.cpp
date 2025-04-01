#include "pch.h"
#include "Engine.h"

void Engine::Init(const WindowInfo& info)
{
	_window = info;

	_viewPort = { 0, 0, static_cast<FLOAT>(info.width), static_cast<FLOAT>(info.height), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, info.width, info.height);		
	
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

	_input->Init(info.hwnd);
	_timer->Init();

	ResizeWindow(info.width, info.height);
}

void Engine::Render()
{
	Update();

	RenderBegin();

	// TODO
	_shader->Update();

	static Transform t;

	if (INPUT->GetButton(KEY_TYPE::W))
		t.offset.y += 1.0f * DELTA_TIME;
	if (INPUT->GetButton(KEY_TYPE::S))
		t.offset.y -= 1.0f * DELTA_TIME;
	if (INPUT->GetButton(KEY_TYPE::A))
		t.offset.x -= 1.0f * DELTA_TIME;
	if (INPUT->GetButton(KEY_TYPE::D))
		t.offset.x += 1.0f * DELTA_TIME;

	_mesh->SetTransform(t);

	_mesh->SetTexture(_texture);

	_mesh->Render();

	RenderEnd();
}

void Engine::Update()
{
	_input->Update();
	_timer->Update();

	ShowFps();
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

void Engine::ShowFps()
{
	uint32 fps = _timer->GetFps();

	WCHAR text[100] = L"";
	wsprintf(text, L"FPS: %d", fps);

	SetWindowText(_window.hwnd, text);
}
