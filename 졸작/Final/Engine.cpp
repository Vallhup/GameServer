#include "pch.h"
#include "Engine.h"
#include "DX12Core.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "Timer.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexIndexBuffer.h"
#include "Importer.h"
#include "Camera.h"

#include "Input.h"

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

    graphics = make_unique<DX12Core>();
    graphics->Initialize(mHwnd);

    sManager = make_unique<SceneManager>();
    sManager->Initialize(*graphics);

    nManager = make_unique<NetworkManager>();
    nManager->Initialize();

    graphics->FlushCommandQueue();

    Input::Initialize(nManager.get());
}

void Engine::Update(const float deltaTime)
{
    sManager->ProcessPendingSceneChange(*graphics);
    sManager->Update(deltaTime);

    nManager->Update();
}

void Engine::Render()
{
    graphics->RenderBegin(viewport, scissorRect);

    //graphics->BeginShadowPass();
    //TODO : RenderShadow();
    //graphics->EndShadowPass();

    // 1. Deferred G-Buffer Pass (불투명 머티리얼만)
    graphics->BeginGBufferPass();
    sManager->RenderDeferred();  // 불투명한 것들만
    graphics->EndGBufferPass();

    // 2. Deferred Lighting Pass  
    graphics->BeginLightingPass();
    graphics->RenderFullscreenQuad();

    // 3. Forward Alpha Pass (투명 머티리얼)
    // 백버퍼 + depth buffer 사용, alpha blending 활성화
    graphics->BeginForwardPass();
    sManager->RenderEffects();   // 이펙트를 먼저 그려야 머리카락이 안없어짐
    sManager->RenderForward();   // 머리카락 등 투명한 것들

    graphics->RenderEnd();

    ShowFps();
}

void Engine::Shutdown()
{
    if (sManager && sManager->GetCurrentScene())
    {
        auto camera = sManager->GetCurrentScene()->GetCamera();
        if (camera)
        {
            camera->ReleaseMouse();
            OutputDebugStringA("Mouse Released!! \n");
        }
    }

    if (graphics)
        graphics->GetSwapChain()->SetFullscreenState(FALSE, nullptr);
    sManager->Release();

    nManager->Release();
}

void Engine::ShowFps()
{
    UINT32 fps = GET(Timer).GetFps();
    WCHAR text[100] = L"";
    wsprintf(text, L"Final      FPS: %d", fps);
    SetWindowText(mHwnd, text);
}