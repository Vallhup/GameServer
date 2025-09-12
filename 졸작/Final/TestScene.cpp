#include "pch.h"
#include "TestScene.h"
#include "DX12Core.h"
#include "GameObject.h"
#include "MainCharacter.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Animator.h"
#include "Camera.h"

TestScene::~TestScene() = default;

void TestScene::Release()
{

}

void TestScene::Reset()
{
	knight.reset();

	Material::Cleanup();
	OutputDebugStringA("TestScene Data has been deleted!! \n----------------------------------------\n");
}

const float* TestScene::GetBackgroundColor()
{
	return Colors::LightBlue;
}

void TestScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nTestScene Data has been created!! \n");

	{
		knight = make_shared<MainCharacter>();
		auto meshrenderer = knight->AddComponent<MeshRenderer>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		meshrenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");
		transform->SetInitPosition(0.f, 0.f, 0.f);
		transform->SetRotation(-1.57f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		meshrenderer->ReleaseUploadBuffers();

		knight->SetCamera(cam.get());
	}
}

void TestScene::UpdateScene(const float deltaTime)
{
	{
		knight->Update(deltaTime);
	}
}

void TestScene::RenderSceneDeferred()
{
	{
		if (knight)
		{
			auto meshrenderer = knight->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->RenderDeferred(*coreRef);
		}
	}
}

void TestScene::RenderSceneForward()
{
	{
		if (knight)
		{
			auto meshrenderer = knight->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->RenderForward(*coreRef);
		}
	}
}

void TestScene::RenderSceneEffects()
{
}

int TestScene::GetSceneWidth() const
{
	return 0;
}

void TestScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Login);
	}
}
