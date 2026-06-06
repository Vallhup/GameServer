#include "pch.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Input.h"
#include "TitleScene.h"
#include "SelectScene.h"
#include "PlazaScene.h"
#include "FirstBattleScene.h"
#include "SecondBattleScene.h"
#include "FinalBattleScene.h"
#include "LoadingScene.h"
#include "Material.h"
#include "ResourceManager.h"
#include "UIManager.h"
#include "LightManager.h"
#include "SkyBox.h"
#include "Camera.h"
#include "ImGuiManager.h"
#include "ShadowMappingManager.h"
#include "SoundManager.h"

SceneManager::~SceneManager()
{
	Release();
}

void SceneManager::Initialize(HWND hWnd, DX12Core& core)
{
    hwnd = hWnd;

    RegisterScene<TitleScene>(SceneType::Title);
    RegisterScene<SelectScene>(SceneType::Select);
    RegisterScene<PlazaScene>(SceneType::Plaza);
    RegisterScene<FirstBattleScene>(SceneType::Village);
    RegisterScene<SecondBattleScene>(SceneType::Castle);
    RegisterScene<FinalBattleScene>(SceneType::Final);

    RegisterScene<LoadingScene>(SceneType::Loading);

    sceneRenderer = make_unique<SceneRenderer>();
    sceneRenderer->Initialize(core.GetDevice());

    SceneStart(core);
}

void SceneManager::Update(const float deltaTime)
{
    if (mCurrentScene)
    {
        mCurrentScene->Update(deltaTime);
    }
}

void SceneManager::BeginRender()
{
    if (sceneRenderer)
        sceneRenderer->BeginFrame();
}

void SceneManager::RenderDeferred()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderSceneDeferred();
    }
}

void SceneManager::RenderForward()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderSceneForward();
    }
}

void SceneManager::RenderShadowStatic()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderSceneShadowStatic();
    }
}

void SceneManager::RenderShadowDynamic()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderSceneShadowDynamic();
    }
}

void SceneManager::RenderEffects()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderSceneEffects();
    }
}

void SceneManager::Release()
{
    if (mCurrentScene)
        mCurrentScene->Release();

    mCurrentScene = nullptr;

    for (auto& scene : mScenes)
    {
        if (scene)
        {
            scene.reset();     
        }
    }

    RESOURCE.ClearCache();
    Material::Cleanup();

    sceneRenderer->ReleaseUploadBuffer();
}

Scene* SceneManager::GetCurrentScene() const
{
	return mCurrentScene;
}

SceneType SceneManager::GetCurrentSceneType() const
{
    return currSceneType;
}

SceneRenderer* SceneManager::GetSceneRenderer() const
{
    return sceneRenderer.get();
}

void SceneManager::SceneStart(DX12Core& core)
{
    size_t index = static_cast<size_t>(SceneType::Title);

    if (static_cast<size_t>(SceneType::END) == index)
    {
        PostQuitMessage(0);
        return;
    }

    currSceneType = SceneType::Title;

    mCurrentScene = mScenes[index].get();
    mCurrentScene->SetSceneManager(this);
    mCurrentScene->Initialize(hwnd, core);
    ApplySceneSettings(core);
    UI_MANAGER->SetCurrentScene(currSceneType);
}

void SceneManager::RequestSceneChange(SceneType type)
{
    pendingSceneChange = true;
    nextSceneType = type;
}

void SceneManager::RequestLoadingScene(SceneType targetSceneType)
{
    auto* loading = static_cast<LoadingScene*>(mScenes[(size_t)SceneType::Loading].get());
    loading->SetTargetScene(targetSceneType);
    RequestSceneChange(SceneType::Loading);
}

void SceneManager::ProcessPendingSceneChange(DX12Core& core)
{
    if (!pendingSceneChange) return;

    pendingSceneChange = false;

    if (mCurrentScene)
    {
        SOUND_MANAGER->StopAllSFX();

        IMGUI.SetMyPlayer(nullptr);
        IMGUI.SetSkyBox(nullptr);
        IMGUI.SetCamera(nullptr);

        mCurrentScene->Release();
    }

    core.ResetCommandQueue();

    size_t index = static_cast<size_t>(nextSceneType);
    mCurrentScene = mScenes[index].get();
    mCurrentScene->SetSceneManager(this);
    
    MoveInstancingBatches(nextSceneType);

    mCurrentScene->Initialize(hwnd, core);
    currSceneType = nextSceneType;
    ApplySceneSettings(core);
    UI_MANAGER->SetCurrentScene(nextSceneType);

    if (nextSceneType != SceneType::Loading)
    {
        auto& transition = ENGINE.GetWorldTransitionController();
        if (transition.HasPendingReady())
        {
            const uint64_t transferId = transition.GetTransferId();

            if (NETWORK_MANAGER &&
                NETWORK_MANAGER->SendWorldTransitionReadyPacket(transferId))
            {
                transition.MarkReadySent();
                transition.Complete();
            }
            else
            {
                transition.Reset();
            }
        }
    }

    core.FlushCommandQueue();
}

void SceneManager::MoveInstancingBatches(SceneType type)
{
    auto* loading = static_cast<LoadingScene*>(mScenes[(size_t)SceneType::Loading].get());
    auto batches = loading->TakeBatches(type);
    mCurrentScene->SetInstancingBatches(move(batches));
}

void SceneManager::ApplySceneSettings(DX12Core& core)
{
    if (!mCurrentScene) return;

    if (currSceneType == SceneType::Loading ||
        currSceneType == SceneType::Title ||
        currSceneType == SceneType::Select) return;

    const SceneSettings s = mCurrentScene->GetSceneSettings();

    if (auto* sky = core.GetLightMgr()->GetSkyBox())
    {
        auto& sun = sky->GetSun();
        sun.direction = s.light.sunDirection;
        sun.color = s.light.sunColor;
        sun.intensity = s.light.sunIntensity;
    }

    core.GetLightMgr()->UpdateLights();

    if (auto* cam = mCurrentScene->GetCamera()) {
        cam->SetLutPreset(s.lut.lutIndex, s.lut.saturation);
    }

    auto& vf = core.GetVolumetricFogData();
    vf.density = s.fog.density;
    vf.scattering = s.fog.scattering;
    vf.absorption = s.fog.absorption;
    vf.hgAnisotropy = s.fog.hgAnisotropy;
    vf.maxSteps = s.fog.maxSteps;
    vf.maxDistance = s.fog.maxDistance;
    vf.jitterStrength = s.fog.jitterStrength;
    vf.heightFalloff = s.fog.heightFalloff;
    vf.groundHeight = s.fog.groundHeight;
    vf.lightColor = s.fog.lightColor;
    vf.lightIntensity = s.fog.lightIntensity;
    core.UpdateVolumetricFog();

    if (auto* sky = core.GetLightMgr()->GetSkyBox()) {
        auto& sc = sky->GetConstants();
        sc.skyTintColor = s.skybox.tintColor;
        sc.skyExposure = s.skybox.exposure;
        sc.skySaturation = s.skybox.saturation;
        sky->UpdateConstants();
    }

    if (auto* sm = core.GetShadowMgr()) {
        auto& cs = sm->GetCsmConstants();
        cs.shadowAmbientMin = s.shadow.shadowAmbientMin;
        cs.shadowFloor = s.shadow.shadowFloor;
        sm->UploadCsmConstants();
    }
}
