#include "pch.h"
#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "Camera.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "DX12Graphics.h"
#include "Animator.h"

GameScene::~GameScene() = default;

void GameScene::Release()
{
}

void GameScene::Reset()
{
}

void GameScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

const float* GameScene::GetBackgroundColor()
{
	return Colors::Snow;
}

void GameScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	OutputDebugStringA("----------------------------------------\nGameScene Data has been created!! \n");
	GET(Camera).Initialize();

	{
		Dragon = make_shared<GameObject>();
		auto meshRenderer = Dragon->AddComponent<MeshRenderer>();
		auto transform = Dragon->AddComponent<Transform>();
		auto animator = Dragon->AddComponent<Animator>();
		meshRenderer->SetMesh(L"../FBXOutput/Dragon");
		transform->SetPosition(0.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(Dragon);

		OutputDebugStringA("Dragon created!!\n");
	}

	{
		knight = make_shared<GameObject>();
		auto meshRenderer = knight->AddComponent<MeshRenderer>();
		auto transform = knight->AddComponent<Transform>();
		meshRenderer->SetMesh(L"../FBXOutput/knight");
		transform->SetPosition(1.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(knight);

		OutputDebugStringA("Strut created!!\n");
	}

	OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
	GET(DX12Graphics).FlushCommandQueue();  
	GET(DX12Graphics).ResetCommandQueue();
	
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->ReleaseUploadBuffers();
	}
	OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");
}

void GameScene::UpdateScene(const float deltaTime)
{
	GET(Camera).Update(deltaTime);

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);
}

void GameScene::RenderScene()
{
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->Render();
	}
}

int GameScene::GetSceneWidth() const
{
	return 0;
}
