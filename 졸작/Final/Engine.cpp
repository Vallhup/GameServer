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
#include "DeviceContext.h"
#include "SwapChain.h"
#include "ShadowMappingManager.h"
#include "RenderTargets.h"
#include "LightManager.h"
#include "FroxelManager.h"
#include "SSAO.h"
#include "LookUpTextures.h"

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
    sceneManager->Initialize(mHwnd, *graphics);

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

    graphics->Update();

    effectManager->Update(deltaTime);

    uiManager->Update(deltaTime);

    soundManager->Update();
}

void Engine::Render()
{
    graphics->RenderBegin(viewport, scissorRect);

    IMGUI.BeginFrame();

    sceneManager->BeginRender();

    for (int i = 0; i < graphics->GetShadowMgr()->GetCascadeCount(); ++i) {
        graphics->BeginShadowPass(i);
        sceneManager->RenderShadow();
        graphics->EndShadowPass(viewport, scissorRect, i);
    }

    graphics->BeginGBufferPass();
    sceneManager->RenderDeferred();  
    graphics->EndGBufferPass();

    static bool ssaoOn = false;

    if (INPUT.GetKeyDown('L'))
    {
        ssaoOn = !ssaoOn;
    }
    
    if (ssaoOn)
    {
        graphics->BeginSsaoPass();
        graphics->EndSsaoPass(viewport, scissorRect);
        graphics->BeginSsaoBlurPass();
        graphics->EndSsaoBlurPass(viewport, scissorRect);
    }
    else
        graphics->ClearSsaoRT();
    

    graphics->BeginLightingPass();
    graphics->RenderFullscreenQuad();

    graphics->BeginForwardPass();
    sceneManager->RenderEffects();   
    sceneManager->RenderForward();   

    uiManager->Render(graphics->GetGraphicsCmdList(), graphics->GetCmdQueue(), viewport);

    IMGUI.DrawDebugUI();
    IMGUI.DrawLoginUI();
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