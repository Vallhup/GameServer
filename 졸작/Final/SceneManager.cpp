#include "pch.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Importer.h"
#include "Input.h"
#include "Timer.h"
#include "Texture.h"
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
        mCurrentScene->RenderDeferred();
    }
}

void SceneManager::RenderForward()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderForward();
    }
}

void SceneManager::RenderShadow()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderShadow();
    }
}

void SceneManager::RenderEffects()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderEffects();
    }
}

void SceneManager::Release()
{
    if (mCurrentScene)
        mCurrentScene->Reset();

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
        mCurrentScene->Reset();
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

    auto& dl = core.GetLightMgr()->GetDeferredLightData();
    dl.lights[0].position = s.light.sunDirection;
    dl.lights[0].color = s.light.sunColor;
    dl.lights[0].intensity = s.light.sunIntensity;

    dl.lights[1].position = s.light.dir2Direction;
    dl.lights[1].color = s.light.dir2Color;
    dl.lights[1].intensity = s.light.dir2Intensity;

    dl.lights[2].position = s.light.pointPosition;
    dl.lights[2].range = s.light.pointRange;
    dl.lights[2].color = s.light.pointColor;
    dl.lights[2].intensity = s.light.pointIntensity;

    auto& fl = core.GetLightMgr()->GetForwardLightData();
    fl.direction = s.light.sunDirection;
    fl.color = s.light.sunColor;
    fl.intensity = s.light.sunIntensity;

    if (auto* sky = core.GetLightMgr()->GetSkyBox()) {
        sky->GetSun().direction = s.light.sunDirection;
        sky->GetSun().color = s.light.sunColor;
        sky->GetSun().intensity = s.light.sunIntensity;
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
}
