#include "pch.h"
#include "StartScene.h"
#include "Camera.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Transform.h"

StartScene::~StartScene() = default;

void StartScene::Release()
{
}

void StartScene::Reset()
{
}

const float* StartScene::GetBackgroundColor()
{
	return Colors::LightBlue;
}

void StartScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
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

void StartScene::UpdateLogic(const float deltaTime)
{
	GET(Camera).Update(deltaTime);

	for (int i = 0; i < 10; ++i)
	{
		for (int j = 0; j < 10; ++j)
			knights[j + (i * 10)]->Update(deltaTime);
	}
}

void StartScene::RenderScene()
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

int StartScene::GetSceneWidth() const
{
	return 0;
}
