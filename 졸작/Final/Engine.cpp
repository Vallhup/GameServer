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
#include "ClusterLightManager.h"
#include "SSAO.h"
#include "LookUpTextures.h"
#include "BloomManager.h"

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

    inboundQueue = make_unique<ClientInboundPacketQueue>();

    networkManager = make_unique<NetworkManager>();
    networkManager->Initialize(1, ip, port, listener);

    soundManager = make_unique<SoundManager>();
    soundManager->Initialize();

    effectManager = make_unique<EffectManager>();
    effectManager->Initialize(*graphics);

    graphics->FlushCommandQueue();
}

void Engine::Update(const float deltaTime)
{
    inboundQueue->Drain(inboundPackets);
    for (auto& packet : inboundPackets)
    {
        packetRouter.Route(packet);
    }

    ProcessWorldTransitionState();

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

    static bool ssaoOn = true;

    if (INPUT.GetKeyDown('L'))
    {
        ssaoOn = !ssaoOn;
    }
    
    if (ssaoOn)
    {
        graphics->SsaoPass();
        graphics->SsaoBlurPass(viewport, scissorRect);
    }
    else
        graphics->ClearSsaoRT();
    
    graphics->FogPass(viewport, scissorRect);

    graphics->LightingPass();

    graphics->ForwardPass();
    sceneManager->RenderForward();
    sceneManager->RenderEffects();

    graphics->BloomPass();

    graphics->BlitPass();

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
            OutputDebugStringA("Mouse Released!!\n");
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

void Engine::ProcessWorldTransitionState()
{
    auto& transition = worldTransitionController;

    if (transition.GetPhase() != ClientWorldTransitionPhase::BeginReceived)
    {
        return;
    }

    const uint32_t targetWorldDefId = transition.GetTargetWorldDefId();

    SceneType targetScene;
    switch (targetWorldDefId) {
    case 1:     targetScene = SceneType::Plaza;   break;
    case 2:     targetScene = SceneType::Village; break;
    case 3:     targetScene = SceneType::Castle;  break;
    case 4:     targetScene = SceneType::Final;   break;
    default:    targetScene = SceneType::Plaza;   break;
    }

    sceneManager->RequestLoadingScene(targetScene);
    transition.MarkLoadingStarted();
}
