#include "pch.h"
#include "SceneManager.h"
#include "DX12Core.h"
#include "Importer.h"
#include "Input.h"
#include "Timer.h"
#include "Texture.h"
#include "TestScene.h"
#include "LoginScene.h"
#include "ServerSquareScene.h"
#include "GameScene.h"
#include "Camera.h"
#include "ServerTestScene.h"

SceneManager::~SceneManager()
{
	Release();
}

void SceneManager::Initialize(DX12Core& core)
{
    RegisterScene<TestScene>(SceneType::Start);
    RegisterScene<LoginScene>(SceneType::Login);
    RegisterScene<ServerSquareScene>(SceneType::ServerSquare);
    RegisterScene<GameScene>(SceneType::MainGame);
    RegisterScene<ServerTestScene>(SceneType::Scene1);
 
    SceneStart(core);
}

void SceneManager::Update(const float deltaTime)
{
    if (mCurrentScene)
    {
        mCurrentScene->Update(deltaTime);
    }
}

void SceneManager::Render()
{
    if (mCurrentScene)
    {
        mCurrentScene->RenderDeferred();
    }
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
}

Scene* SceneManager::GetCurrentScene() const
{
	return mCurrentScene;
}

void SceneManager::SceneStart(DX12Core& core)
{
    size_t index = static_cast<size_t>(SceneType::Start);

    if (static_cast<size_t>(SceneType::END) == index)
    {
        PostQuitMessage(0);
        return;
    }

    mCurrentScene = mScenes[index].get();
    mCurrentScene->SetSceneManager(this);
    mCurrentScene->Initialize(core);

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
    mCurrentScene->Initialize(core);

    core.FlushCommandQueue();

    core.SetBackgroundColor(mCurrentScene->GetBackgroundColor());
}
