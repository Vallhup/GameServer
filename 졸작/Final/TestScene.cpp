#include "pch.h"
#include "TestScene.h"
#include "DX12Core.h"
#include "GameObject.h"
#include "MainCharacter.h"
#include "Transform.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Animator.h"
#include "Camera.h"
#include "Mesh.h"
#include "SceneRenderer.h"

TestScene::~TestScene() = default;

void TestScene::Release()
{

}

void TestScene::Reset()
{
	gameObjects.clear();
	knight.reset();

	Material::ReleaseUploadBuffers();
	OutputDebugStringA("TestScene Data has been deleted!! \n----------------------------------------\n");
}

const float* TestScene::GetBackgroundColor()
{
	return Colors::LightBlue;
}

void TestScene::InitializeSceneObjectPools()
{
}

void TestScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nTestScene Data has been created!! \n");

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

void TestScene::UpdateScene(const float deltaTime)
{	
	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);
}

void TestScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

void TestScene::RenderSceneForward()
{
	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void TestScene::RenderSceneShadow()
{
	sManagerRef->GetSceneRenderer()->RenderShadow(*coreRef, gameObjects);
}

void TestScene::RenderSceneEffects()
{
}

void TestScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Login);
	}
}
