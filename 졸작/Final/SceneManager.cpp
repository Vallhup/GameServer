#include "pch.h"
#include "SceneManager.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "CommandQueue.h"
#include "Importer.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Input.h"
#include "Timer.h"
#include "DescriptorHeap.h"
#include "Texture.h"
#include "Camera.h"
#include "TestScene.h"
#include "LoginScene.h"
#include "ServerSquareScene.h"
#include "GameScene.h"

SceneManager& SceneManager::Get()
{
	static SceneManager sceneManager;
	return sceneManager;
}

SceneManager::~SceneManager()
{
	Release();
}

void SceneManager::Initialize(HWND hwnd)
{
	mHwnd = hwnd;

    RegisterScene<TestScene>(SceneType::Start);
    RegisterScene<LoginScene>(SceneType::Login);
    RegisterScene<ServerSquareScene>(SceneType::ServerSquare);
    RegisterScene<GameScene>(SceneType::MainGame);
 
    SceneStart();
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
        mCurrentScene->Render();
    }
}

void SceneManager::Release()
{
    for (auto& scene : mScenes)
    {
        if (scene)
        {
            scene->Release();  
            scene.reset();     
        }
    }

    mCurrentScene = nullptr;
}

Scene* SceneManager::GetCurrentScene() const
{
	return mCurrentScene;
}

void SceneManager::SceneStart()
{
    size_t index = static_cast<size_t>(SceneType::Start);

    if (static_cast<size_t>(SceneType::END) == index)
    {
        PostQuitMessage(0);
        return;
    }

    mCurrentScene = mScenes[index].get();
    mCurrentScene->Initialize(
        GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get()
    );

    GET(DX12Graphics).GetCmdQueue()->SetBackgroundColor(mCurrentScene->GetBackgroundColor());
}

void SceneManager::RequestSceneChange(SceneType type)
{
    pendingSceneChange = true;
    nextSceneType = type;
}

void SceneManager::ProcessPendingSceneChange()
{
    if (!pendingSceneChange) return;

    pendingSceneChange = false;

    if (mCurrentScene)
    {
        mCurrentScene->Reset();
    }

    GET(DX12Graphics).ResetCommandQueue();

    size_t index = static_cast<size_t>(nextSceneType);
    mCurrentScene = mScenes[index].get();
    mCurrentScene->Initialize(
        GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get()
    );

    GET(DX12Graphics).FlushCommandQueue();

    GET(DX12Graphics).GetCmdQueue()->SetBackgroundColor(mCurrentScene->GetBackgroundColor());
}
