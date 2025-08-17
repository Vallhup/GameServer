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
#include "Material.h"

GameScene::~GameScene() = default;

void GameScene::Release()
{
}

void GameScene::Reset()
{
	Material::Cleanup();
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
		dragon = make_shared<GameObject>();
		auto meshRenderer = dragon->AddComponent<MeshRenderer>();
		auto transform = dragon->AddComponent<Transform>();
		auto animator = dragon->AddComponent<Animator>();
		meshRenderer->SetMesh(L"../FBXOutput/Dragon");
		transform->SetPosition(0.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(dragon);

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

	for (int i = 1; i < 10; ++i) {
		auto newKnight = make_shared<GameObject>();
		auto meshRenderer = newKnight->AddComponent<MeshRenderer>();
		auto transform = newKnight->AddComponent<Transform>();
		meshRenderer->SetMesh(L"../FBXOutput/knight");
		transform->SetPosition(i * 1.5f + 1.0f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(newKnight);
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

	// ★ Dragon 애니메이션 전환 (키 중복 방지)
	//if (dragon) {
	//	auto animator = dragon->GetComponent<Animator>();
	//	if (animator) {
	//		// 1번 키
	//		bool key1Current = (GetAsyncKeyState('1') & 0x8000) != 0;
	//		if (key1Current && !key1Pressed) {
	//			animator->PlayAnimation(0);  // Fly
	//			OutputDebugStringA("Dragon Animation 0 (Fly) played!\n");
	//		}
	//		key1Pressed = key1Current;

	//		// 2번 키
	//		bool key2Current = (GetAsyncKeyState('2') & 0x8000) != 0;
	//		if (key2Current && !key2Pressed) {
	//			animator->PlayAnimation(1);  // Idle
	//			OutputDebugStringA("Dragon Animation 1 (Idle) played!\n");
	//		}
	//		key2Pressed = key2Current;

	//		// 3번 키
	//		bool key3Current = (GetAsyncKeyState('3') & 0x8000) != 0;
	//		if (key3Current && !key3Pressed) {
	//			animator->PlayAnimation(2);  // Run
	//			OutputDebugStringA("Dragon Animation 2 (Run) played!\n");
	//		}
	//		key3Pressed = key3Current;

	//		// 4번 키
	//		bool key4Current = (GetAsyncKeyState('4') & 0x8000) != 0;
	//		if (key4Current && !key4Pressed) {
	//			animator->PlayAnimation(3);  // Walk
	//			OutputDebugStringA("Dragon Animation 3 (Walk) played!\n");
	//		}
	//		key4Pressed = key4Current;
	//	}
	//}

	if (dragon) {
		auto animator = dragon->GetComponent<Animator>();
		if (animator) {
			// 1번 키
			bool key1Current = (GetAsyncKeyState('1') & 0x8000) != 0;
			if (key1Current && !key1Pressed) {
				animator->TransitionToAnimation(0, 0.6f);  // Fly
				OutputDebugStringA("Dragon Animation 0 (Fly) played!\n");
			}
			key1Pressed = key1Current;

			// 2번 키
			bool key2Current = (GetAsyncKeyState('2') & 0x8000) != 0;
			if (key2Current && !key2Pressed) {
				animator->TransitionToAnimation(1, 0.4f);  // Idle
				OutputDebugStringA("Dragon Animation 1 (Idle) played!\n");
			}
			key2Pressed = key2Current;

			// 3번 키
			bool key3Current = (GetAsyncKeyState('3') & 0x8000) != 0;
			if (key3Current && !key3Pressed) {
				animator->TransitionToAnimation(2, 0.4f);  // Run
				OutputDebugStringA("Dragon Animation 2 (Run) played!\n");
			}
			key3Pressed = key3Current;

			// 4번 키
			bool key4Current = (GetAsyncKeyState('4') & 0x8000) != 0;
			if (key4Current && !key4Pressed) {
				animator->TransitionToAnimation(3, 0.4f);  // Walk
				OutputDebugStringA("Dragon Animation 3 (Walk) played!\n");
			}
			key4Pressed = key4Current;
		}
	}

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
