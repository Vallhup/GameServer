#include "pch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "Timer.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexIndexBuffer.h"
#include "Importer.h"
#include "Input.h"
#include "SoundManager.h"
#include "EffectManager.h"
#include "ImGuiManager.h"
#include "UIManager.h"

Engine& Engine::Get()
{
    static Engine engine;
    return engine;
}

void Engine::Initialize(HWND hwnd, string_view ip, uint16 port, IConnectionListener& listener)
{
    mHwnd = hwnd;

    viewport = { 0, 0, static_cast<FLOAT>(WinSize.x), static_cast<FLOAT>(WinSize.y), 0.0f, 1.0f };
    scissorRect = CD3DX12_RECT(0, 0, WinSize.x, WinSize.y);

    graphics = make_unique<DX12Core>();
    graphics->Initialize(mHwnd);

    uiManager = make_unique<UIManager>();
    uiManager->Initialize(*graphics);

    IMGUI.Initialize(mHwnd, *graphics);

    sceneManager = make_unique<SceneManager>();
    sceneManager->Initialize(hwnd, *graphics);

    networkManager = make_unique<NetworkManager>();
    networkManager->Initialize(1, ip, port, listener);

    soundManager = make_unique<SoundManager>();
    soundManager->Initialize();

    effectManager = make_unique<EffectManager>();
    effectManager->Initialize(*graphics);

    graphics->FlushCommandQueue();

    INPUT.Initialize(networkManager.get());
}

void Engine::Update(const float deltaTime)
{
    sceneManager->ProcessPendingSceneChange(*graphics);
    sceneManager->Update(deltaTime);

    effectManager->Update(deltaTime);

    soundManager->Update();
}

void Engine::Render()
{
    graphics->RenderBegin(viewport, scissorRect);

    IMGUI.BeginFrame();

    sceneManager->BeginRender();

    graphics->BeginShadowPass();
    sceneManager->RenderShadow();
    graphics->EndShadowPass(viewport, scissorRect);

    // 1. Deferred G-Buffer Pass (불투명 머티리얼만)
    graphics->BeginGBufferPass();
    sceneManager->RenderDeferred();  // 불투명한 것들만
    graphics->EndGBufferPass();

    // 1.5. SSAO Pass (차폐도) - 추후 재시도

    // 2. Deferred Lighting Pass  
    graphics->BeginLightingPass();
    graphics->RenderFullscreenQuad();

    // 3. Forward Alpha Pass (투명 머티리얼)
    // 백버퍼 + depth buffer 사용, alpha blending 활성화
    graphics->BeginForwardPass();
    sceneManager->RenderEffects();   // 이펙트를 먼저 그려야 머리카락이 안없어짐
    sceneManager->RenderForward();   // 머리카락 등 투명한 것들

    uiManager->Render(graphics->GetGraphicsCmdList(), graphics->GetCmdQueue(), viewport);

    IMGUI.DrawDebugUI();
    IMGUI.EndFrame(graphics->GetGraphicsCmdList());

    graphics->RenderEnd();

    ShowFps();
}

void Engine::Shutdown()
{
    IMGUI.Shutdown();
    uiManager->Release();

    if (sceneManager && sceneManager->GetCurrentScene())
    {
        auto camera = sceneManager->GetCurrentScene()->GetCamera();
        if (camera)
        {
            camera->ReleaseMouse();
            OutputDebugStringA("Mouse Released!! \n");
        }
    }

    if (graphics)
        graphics->GetSwapChain()->SetFullscreenState(FALSE, nullptr);

    sceneManager->Release();
    networkManager->Release();
    soundManager->Release();
    effectManager->Release();
}

void Engine::ShowFps()
{
    UINT32 fps = TIMER.GetFps();
    WCHAR text[100] = L"";
    wsprintf(text, L"Final      FPS: %d", fps);
    SetWindowText(mHwnd, text);
}