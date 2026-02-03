#include "pch.h"
#include "SceneManager.h"
#include "Importer.h"
#include "Input.h"
#include "Timer.h"
#include "Texture.h"
#include "TestScene.h"
#include "LoginScene.h"
#include "ServerSquareScene.h"
#include "GameScene.h"
#include "Camera.h"
#include "Material.h"
#include "ResourceManager.h"

SceneManager::~SceneManager()
{
	Release();
}

void SceneManager::Initialize(HWND hWnd, DX12Core& core)
{
    hwnd = hWnd;

    RegisterScene<TestScene>(SceneType::Start);
    RegisterScene<LoginScene>(SceneType::Login);
    RegisterScene<ServerSquareScene>(SceneType::ServerSquare);
    RegisterScene<GameScene>(SceneType::MainGame);

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
    size_t index = static_cast<size_t>(SceneType::Start);

    if (static_cast<size_t>(SceneType::END) == index)
    {
        PostQuitMessage(0);
        return;
    }

    currSceneType = SceneType::Start;

    mCurrentScene = mScenes[index].get();
    mCurrentScene->SetSceneManager(this);
    Material::InitializeBindlessSystem(core.GetDevice());
    mCurrentScene->Initialize(hwnd, core);

    core.SetBackgroundColor(mCurrentScene->GetBackgroundColor());
}

void SceneManager::RequestSceneChange(SceneType type)
{
    pendingSceneChange = true;
    nextSceneType = type;
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
    mCurrentScene->Initialize(hwnd, core);
    currSceneType = nextSceneType;

    core.FlushCommandQueue();

    core.SetBackgroundColor(mCurrentScene->GetBackgroundColor());
}
