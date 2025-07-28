#include "pch.h"
#include "SceneManager.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "CommandQueue.h"

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

void SceneManager::ChangeScene(SceneType type)
{
    size_t index = static_cast<size_t>(type);

    if (static_cast<size_t>(SceneType::END) == index)
    {
        PostQuitMessage(0);
        return;
    }

    mCurrentScene = mScenes[index].get();
    
    GET(DX12Graphics).GetCmdQueue()->SetBackgroundColor(mCurrentScene->GetBackgroundColor());

    if (mCurrentScene)
    {
        mCurrentScene->Reset();
    }
}