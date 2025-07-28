#include "pch.h"
#include "Engine.h"
#include "Timer.h"
#include "SceneManager.h"
#include "Device.h"
#include "SwapChain.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "Shader.h"
#include "UploadBuffer.h"
#include "DepthStencilView.h"
#include "VertexIndexBuffer.h"
#include "DX12Graphics.h"

Engine& Engine::Get()
{
    static Engine engine;
    return engine;
}

void Engine::Initialize(HWND hwnd)
{
    mHwnd = hwnd;

    viewport = { 0, 0, static_cast<FLOAT>(WinSize.x), static_cast<FLOAT>(WinSize.y), 0.0f, 1.0f };
    scissorRect = CD3DX12_RECT(0, 0, WinSize.x, WinSize.y);

    GET(DX12Graphics).Initialize(mHwnd);

    SceneManager& sManager = GET(SceneManager);

    sManager.Initialize(mHwnd);

    GET(DX12Graphics).FlushCommandQueue();
}

void Engine::Update(const float deltaTime)
{
    GET(SceneManager).Update(deltaTime);
}

void Engine::Render(const float deltaTime)
{
    GET(DX12Graphics).RenderBegin(viewport, scissorRect);
    
    GET(SceneManager).Render();

    GET(DX12Graphics).RenderEnd();

    ShowFps();
}

void Engine::Shutdown()
{
    GET(SceneManager).Release();
}

void Engine::ShowFps()
{
    UINT32 fps = GET(Timer).GetFps();
    WCHAR text[100] = L"";
    wsprintf(text, L"Final      FPS: %d", fps);
    SetWindowText(mHwnd, text);
}