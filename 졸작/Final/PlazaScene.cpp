#include "pch.h"
#include "PlazaScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "Material.h"
#include "MainCharacter.h"
#include "Animator.h"

void PlazaScene::Release()
{
}

void PlazaScene::Reset()
{
	gameObjects.clear();
	knight.reset();

	OutputDebugStringA("TownScene Data has been deleted!! \n----------------------------------------\n");
}

void PlazaScene::InitializeSceneObjectPools()
{
}

void PlazaScene::InitializeLogic()
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

void PlazaScene::UpdateScene(const float deltaTime)
{
	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, {}, knight);
}

void PlazaScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

void PlazaScene::RenderSceneForward()
{
	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void PlazaScene::RenderSceneShadow()
{
	sManagerRef->GetSceneRenderer()->RenderShadow(*coreRef, gameObjects);
}

void PlazaScene::RenderSceneEffects()
{
}

void PlazaScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestLoadingScene(SceneType::Village);
	}
}
