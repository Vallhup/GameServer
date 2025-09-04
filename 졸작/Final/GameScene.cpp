#include "pch.h"
#include "GameScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameObject.h"
#include "MainCharacter.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Animator.h"
#include "Material.h"
#include "Camera.h"
#include "EffectRenderer.h"

GameScene::~GameScene() = default;

void GameScene::Release()
{
}

void GameScene::Reset()
{
	// TODO: 씬 데이터 리셋 코드 추가
	dragon.reset();
	knight.reset();
	gameObjects.clear();

	Material::Cleanup();
	OutputDebugStringA("GameScene Data has been deleted!! \n----------------------------------------\n");
}

void GameScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

const float* GameScene::GetBackgroundColor()
{
	return Colors::Snow;
}

void GameScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nGameScene Data has been created!! \n");

	/*{
		dragon = make_shared<GameObject>();
		auto meshRenderer = dragon->AddComponent<MeshRenderer>();
		auto transform = dragon->AddComponent<Transform>();
		auto animator = dragon->AddComponent<Animator>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/Dragon");
		transform->SetPosition(0.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.1f, 0.1f, 0.1f);
		AddGameObject(dragon);

		OutputDebugStringA("Dragon created!!\n");
	}*/

	{
		knight = make_shared<MainCharacter>();
		auto meshRenderer = knight->AddComponent<MeshRenderer>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");
		transform->SetPosition(1.f, 0.f, 0.5f);
		transform->SetRotation(-1.57f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		knight->SetCamera(cam.get());
		AddGameObject(knight);

		OutputDebugStringA("Strut created!!\n");
	}

	vector<wstring> names = { L"bookshelf", L"candle", L"chair", L"pillar", L"statue1", L"statue2", L"statue3", L"table", L"throne" };
	for (int i = 1; i < 29; ++i)
	{
		auto map = make_shared<GameObject>();
		auto meshRenderer = map->AddComponent<MeshRenderer>();
		auto transform = map->AddComponent<Transform>();
		if (i < 10)
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/map_cathedral_0" + to_wstring(i));
		else if (i < 20)
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/map_cathedral_" + to_wstring(i));
		else
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/map_cathedral_" + names[i - 20]);
		transform->SetPosition(0.0f, 0.0f, 0.0f);
		transform->SetRotation(0.0f, 0.0f, 0.0f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(map);

		OutputDebugStringA("Strut created!!\n");
	}

	{
		flameEffect = make_shared<GameObject>();
		auto effectRenderer = flameEffect->AddComponent<EffectRenderer>();
		auto transform = flameEffect->AddComponent<Transform>();

		effectRenderer->Initialize(*coreRef);
		effectRenderer->LoadEffect(u"../Effects/Atmosphere.efk");
		transform->SetPosition(1.0f, 1.0f, -15.0f);

		AddGameObject(flameEffect);
	}

	/*{
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
	}*/

	OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();
	
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->ReleaseUploadBuffers();
	}
	OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");
}

void GameScene::UpdateScene(const float deltaTime)
{
	//if (dragon) {
	//	auto animator = dragon->GetComponent<Animator>();
	//	if (animator) {
	//		if (GET(Input).GetKeyDown('1')) {
	//			animator->TransitionToAnimation(0, 0.6f);  // Fly
	//			OutputDebugStringA("Dragon Animation 0 (Fly) played!\n");
	//		}

	//		if (GET(Input).GetKeyDown('2')) {
	//			animator->TransitionToAnimation(1, 0.4f);  // Idle
	//			OutputDebugStringA("Dragon Animation 1 (Idle) played!\n");
	//		}

	//		if (GET(Input).GetKeyDown('3')) {
	//			animator->TransitionToAnimation(2, 0.4f);  // Run
	//			OutputDebugStringA("Dragon Animation 2 (Run) played!\n");
	//		}

	//		if (GET(Input).GetKeyDown('4')) {
	//			animator->TransitionToAnimation(3, 0.4f);  // Walk
	//			OutputDebugStringA("Dragon Animation 3 (Walk) played!\n");
	//		}
	//	}
	//}

	if (flameEffect) {
		auto effectRenderer = flameEffect->GetComponent<EffectRenderer>();
		if (GET(Input).GetKeyDown('5'))
			effectRenderer->PlayEffect();
	}

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);
}

void GameScene::RenderSceneDeferred()
{
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->RenderDeferred(*coreRef);
	}
}

void GameScene::RenderSceneForward()
{
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->RenderForward(*coreRef);
	}
}

void GameScene::RenderSceneEffects()
{
	for (const auto& obj : gameObjects)
	{
		if (auto effectRenderer = obj->GetComponent<EffectRenderer>())
			effectRenderer->Render(*coreRef, cam.get());
	}
}

int GameScene::GetSceneWidth() const
{
	return 0;
}

void GameScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Scene1);
	}
}
