#include "pch.h"
#include "SceneManager.h"
#include "StartScene.h"
#include "MenuScene.h"
#include "RCScene.h"
#include "TGScene.h"
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

    RegisterScene<StartScene>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::Start);
    RegisterScene<MenuScene>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::Menu);
    RegisterScene<RollerCoster>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::Scene1);
    RegisterScene<TankGame>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::Scene2);

    ChangeScene(SceneType::Start);
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