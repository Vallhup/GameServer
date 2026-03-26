#include "pch.h"
#include "TownScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "Material.h"
#include "MainCharacter.h"
#include "Animator.h"

TownScene::~TownScene() = default;

void TownScene::Release()
{
}

void TownScene::Reset()
{
	gameObjects.clear();
	knight.reset();

	Material::ReleaseUploadBuffers();
	OutputDebugStringA("TownScene Data has been deleted!! \n----------------------------------------\n");
}

void TownScene::InitializeSceneObjectPools()
{
}

void TownScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nTownScene Data has been created!! \n");

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
}

void TownScene::UpdateScene(const float deltaTime)
{
	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, {}, knight);
}

void TownScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

void TownScene::RenderSceneForward()
{
	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void TownScene::RenderSceneShadow()
{
	sManagerRef->GetSceneRenderer()->RenderShadow(*coreRef, gameObjects);
}

void TownScene::RenderSceneEffects()
{
}

void TownScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestLoadingScene(SceneType::MainGame);
	}
}
