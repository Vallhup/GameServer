#include "pch.h"
#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "Camera.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Transform.h"

GameScene::~GameScene() = default;

void GameScene::Release()
{
}

void GameScene::Reset()
{
}

const float* GameScene::GetBackgroundColor()
{
	return Colors::Snow;
}

void GameScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	GET(Camera).Initialize();

	Dragon = make_shared<GameObject>();

	auto meshRenderer = Dragon->AddComponent<MeshRenderer>();
	meshRenderer->SetMesh(L"../FBXOutput/Dragon");

	auto transform = Dragon->AddComponent<Transform>();
	transform->SetPosition(0.f, 0.f, 0.5f);
	transform->SetRotation(0.f, 0.f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);
}

void GameScene::UpdateScene(const float deltaTime)
{
	GET(Camera).Update(deltaTime);

	Dragon->Update(deltaTime);
}

void GameScene::RenderScene()
{
	if (Dragon)
	{
		auto meshRenderer = Dragon->GetComponent<MeshRenderer>();

		if (meshRenderer)
			meshRenderer->Render();

	}	
}

int GameScene::GetSceneWidth() const
{
	return 0;
}
