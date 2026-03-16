#include "pch.h"
#include "SelectScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "Material.h"
#include "MainCharacter.h"
#include "Animator.h"

SelectScene::~SelectScene() = default;

void SelectScene::Release()
{
}

void SelectScene::Reset()
{
	gameObjects.clear();
	knight.reset();
	dragon.reset();

	Material::ReleaseUploadBuffers();
	OutputDebugStringA("SelectScene Data has been deleted!! \n----------------------------------------\n");
}

void SelectScene::InitializeSceneObjectPools()
{
}

void SelectScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nSelectScene Data has been created!! \n");

	{
		knight = make_shared<MainCharacter>();
		auto mesh = knight->AddComponent<Mesh>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");
		transform->SetInitPosition(0.f, 0.f, 0.f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		gameObjects.push_back(knight);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		mesh->ReleaseUploadBuffers();

		knight->SetAsLocalPlayer(cam.get());
	}

	{
		dragon = make_shared<GameObject>();
		auto mesh = dragon->AddComponent<Mesh>();
		auto transform = dragon->AddComponent<Transform>();
		auto animator = dragon->AddComponent<Animator>();

		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/Tank/monster_smallboss");
		transform->SetInitPosition(0.f, 0.f, 0.f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		gameObjects.push_back(dragon);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		mesh->ReleaseUploadBuffers();

		OutputDebugStringA("Dragon created!!\n");
	}
}

void SelectScene::UpdateScene(const float deltaTime)
{
	if (dragon) {
		auto animator = dragon->GetComponent<Animator>();
		if (animator) {
			if (INPUT.GetKeyDown('1')) {
				animator->TransitionToAnimation(0, 0.6f);  // Fly
				OutputDebugStringA("Dragon Animation 0 (Fly) played!\n");
			}

			if (INPUT.GetKeyDown('2')) {
				animator->TransitionToAnimation(1, 0.4f);  // Idle
				OutputDebugStringA("Dragon Animation 1 (Idle) played!\n");
			}

			if (INPUT.GetKeyDown('3')) {
				animator->TransitionToAnimation(2, 0.4f);  // Run
				OutputDebugStringA("Dragon Animation 2 (Run) played!\n");
			}

			if (INPUT.GetKeyDown('4')) {
				animator->TransitionToAnimation(3, 0.4f);  // Walk
				OutputDebugStringA("Dragon Animation 3 (Walk) played!\n");
			}
		}
	}

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, {}, knight);
}

void SelectScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

void SelectScene::RenderSceneForward()
{
	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void SelectScene::RenderSceneShadow()
{
	sManagerRef->GetSceneRenderer()->RenderShadow(*coreRef, gameObjects);
}

void SelectScene::RenderSceneEffects()
{
}

void SelectScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Town);
	}
}
