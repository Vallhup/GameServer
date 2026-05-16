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

    soundManager = make_unique<SoundManager>();
    soundManager->Initialize();

    uiManager = make_unique<UIManager>();
    uiManager->Initialize(*graphics);

    IMGUI.Initialize(mHwnd, *graphics);

    networkManager = make_unique<NetworkManager>();
    networkManager->Initialize(1, ip, port, listener);

    sceneManager = make_unique<SceneManager>();
    sceneManager->Initialize(mHwnd, *graphics);

    inboundQueue = make_unique<ClientInboundPacketQueue>();

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

    auto* shadowMgr = graphics->GetShadowMgr();
    const int staticCascadeIdx = shadowMgr->GetStaticCacheCascadeIndex();

    // Cascade 0, 1: 전 객체 그리기 (Static + Dynamic 분리 호출)
    for (int i = 0; i < staticCascadeIdx; ++i) {
        graphics->BeginShadowPass(i);
        sceneManager->RenderShadowStatic();
        sceneManager->RenderShadowDynamic();
        graphics->EndShadowPass(viewport, scissorRect, i);
    }

    // Cascade 2: static caster cache는 dirty일 때만 재생성, dynamic은 매 프레임 overlay
    if (shadowMgr->IsCascadeDirty(staticCascadeIdx)) {
        graphics->BeginStaticShadowPass();
        sceneManager->RenderShadowStatic();
        graphics->EndStaticShadowPass();
    }
    graphics->CopyStaticToCsmCascade2();
    graphics->BeginDynamicShadowPass();
    sceneManager->RenderShadowDynamic();
    graphics->EndDynamicShadowPass(viewport, scissorRect);

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

    graphics->ClusterLightCullPass();

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
