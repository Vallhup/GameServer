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

    RegisterScene<TestScene>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::Start);
    RegisterScene<LoginScene>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::Login);
    RegisterScene<ServerSquareScene>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::ServerSquare);
    RegisterScene<GameScene>(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), SceneType::MainGame);
 
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
    ChangeScene(SceneType::Start);
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