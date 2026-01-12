#include "pch.h"
#include "LoginScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Camera.h"
#include "MainCharacter.h"
#include "Transform.h"
#include "Animator.h"
#include "Mesh.h"
#include "SceneRenderer.h"

LoginScene::~LoginScene() = default;

void LoginScene::Release()
{
}

void LoginScene::Reset()
{
	gameObjects.clear();
	knight.reset();
	dragon.reset();

	Material::ReleaseUploadBuffers();
	OutputDebugStringA("LoginScene Data has been deleted!! \n----------------------------------------\n");
}

const float* LoginScene::GetBackgroundColor()
{
	return Colors::MediumAquamarine;
}

void LoginScene::InitializeSceneObjectPools()
{
}

void LoginScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nLoginScene Data has been created!! \n");

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

		knight->SetCamera(cam.get());
	}

	{
		dragon = make_shared<GameObject>();
		auto mesh = dragon->AddComponent<Mesh>();
		auto transform = dragon->AddComponent<Transform>();
		auto animator = dragon->AddComponent<Animator>();

		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Dragon/Dragon");
		transform->SetInitPosition(2.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.1f, 0.1f, 0.1f);
		gameObjects.push_back(dragon);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		mesh->ReleaseUploadBuffers();

		OutputDebugStringA("Dragon created!!\n");
	}
}

void LoginScene::UpdateScene(const float deltaTime)
{
	if (dragon) {
		auto animator = dragon->GetComponent<Animator>();
		if (animator) {
			if (GET(Input).GetKeyDown('1')) {
				animator->TransitionToAnimation(0, 0.6f);  // Fly
				OutputDebugStringA("Dragon Animation 0 (Fly) played!\n");
			}

			if (GET(Input).GetKeyDown('2')) {
				animator->TransitionToAnimation(1, 0.4f);  // Idle
				OutputDebugStringA("Dragon Animation 1 (Idle) played!\n");
			}

			if (GET(Input).GetKeyDown('3')) {
				animator->TransitionToAnimation(2, 0.4f);  // Run
				OutputDebugStringA("Dragon Animation 2 (Run) played!\n");
			}

			if (GET(Input).GetKeyDown('4')) {
				animator->TransitionToAnimation(3, 0.4f);  // Walk
				OutputDebugStringA("Dragon Animation 3 (Walk) played!\n");
			}
		}
	}

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);
}

void LoginScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

void LoginScene::RenderSceneForward()
{
	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void LoginScene::RenderSceneShadow()
{
	sManagerRef->GetSceneRenderer()->RenderShadow(*coreRef, gameObjects);
}

void LoginScene::RenderSceneEffects()
{
}

void LoginScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::ServerSquare);
	}
}
