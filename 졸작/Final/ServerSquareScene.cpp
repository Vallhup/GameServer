#include "pch.h"
#include "ServerSquareScene.h"
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

ServerSquareScene::~ServerSquareScene() = default;

void ServerSquareScene::Release()
{
}

void ServerSquareScene::Reset()
{
	gameObjects.clear();
	knight.reset();

	Material::ReleaseUploadBuffers();
	OutputDebugStringA("ServerSquareScene Data has been deleted!! \n----------------------------------------\n");
}

const float* ServerSquareScene::GetBackgroundColor()
{
	return Colors::Pink;
}

void ServerSquareScene::InitializeSceneObjectPools()
{
}

void ServerSquareScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nServerSquareScene Data has been created!! \n");

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
}

void ServerSquareScene::UpdateScene(const float deltaTime)
{
	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);
}

void ServerSquareScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

void ServerSquareScene::RenderSceneForward()
{
	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void ServerSquareScene::RenderSceneShadow()
{
	sManagerRef->GetSceneRenderer()->RenderShadow(*coreRef, gameObjects);
}

void ServerSquareScene::RenderSceneEffects()
{
}

void ServerSquareScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::MainGame);
	}
}
