#include "pch.h"
#include "TestScene.h"
#include "Camera.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Input.h"
#include "SceneManager.h"

TestScene::~TestScene() = default;

void TestScene::Release()
{
}

void TestScene::Reset()
{
}

const float* TestScene::GetBackgroundColor()
{
	return Colors::LightBlue;
}

void TestScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    GET(Camera).Initialize();

	for (int i = 0; i < 10; ++i)
	{
		for (int j = 0; j < 10; ++j)
		{
			knights[j + (i * 10)] = make_shared<GameObject>();

			auto meshRenderer = knights[j + (i * 10)]->AddComponent<MeshRenderer>();
			meshRenderer->SetMesh(L"../FBXOutput/Strut Walking");

			auto transform = knights[j + (i * 10)]->AddComponent<Transform>();
			transform->SetPosition(-0.5f + (i * 0.5f), 0.f, -0.5f + (j * 0.5f));
			transform->SetRotation(-1.57f, 0.f, 0.f);
			transform->SetScale(0.01f, 0.01f, 0.01f);
		}
	}

	/*knight = make_shared<GameObject>();

	auto meshRenderer = knight->AddComponent<MeshRenderer>();
	meshRenderer->SetMesh(L"../FBXOutput/Strut Walking");

	auto transform = knight->AddComponent<Transform>();
	transform->SetPosition(0.f, 0.f, 0.5f);
	transform->SetRotation(-1.57f, 0.f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);*/
}

void TestScene::UpdateScene(const float deltaTime)
{
	GET(Camera).Update(deltaTime);

	for (int i = 0; i < 10; ++i)
	{
		for (int j = 0; j < 10; ++j)
			knights[j + (i * 10)]->Update(deltaTime);
	}

	if (GET(Input).GetKeyDown(VK_TAB))
		GET(SceneManager).ChangeScene(SceneType::Login);
}

void TestScene::RenderScene()
{
	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j)
		{
			if (knights[j + (i * 10)]) {
				auto meshRenderer = knights[j + (i * 10)]->GetComponent<MeshRenderer>();

				if (meshRenderer)
					meshRenderer->Render();
			}
		}
	}
}

int TestScene::GetSceneWidth() const
{
	return 0;
}
