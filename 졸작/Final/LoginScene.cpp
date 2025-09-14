#include "pch.h"
#include "LoginScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Camera.h"
#include "MainCharacter.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Animator.h"

LoginScene::~LoginScene() = default;

void LoginScene::Release()
{
}

void LoginScene::Reset()
{
	knight.reset();
	dragon.reset();

	// Material::Cleanup();
	Material::ReleaseUploadBuffers();
	OutputDebugStringA("LoginScene Data has been deleted!! \n----------------------------------------\n");
}

const float* LoginScene::GetBackgroundColor()
{
	return Colors::MediumAquamarine;
}

void LoginScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nLoginScene Data has been created!! \n");

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

	{
		dragon = make_shared<GameObject>();
		auto meshRenderer = dragon->AddComponent<MeshRenderer>();
		auto transform = dragon->AddComponent<Transform>();
		auto animator = dragon->AddComponent<Animator>();

		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/Dragon");
		transform->SetInitPosition(2.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.1f, 0.1f, 0.1f);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		meshRenderer->ReleaseUploadBuffers();

		OutputDebugStringA("Dragon created!!\n");
	}
}

void LoginScene::UpdateScene(const float deltaTime)
{
	{
		knight->Update(deltaTime);
		dragon->Update(deltaTime);
	}
}

void LoginScene::RenderSceneDeferred()
{
	{
		if (knight)
		{
			auto meshrenderer = knight->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->RenderDeferred(*coreRef);
		}

		if (dragon)
		{
			auto meshrenderer = dragon->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->RenderDeferred(*coreRef);
		}
	}
}

void LoginScene::RenderSceneForward()
{
	{
		if (knight)
		{
			auto meshrenderer = knight->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->RenderForward(*coreRef);
		}

		if (dragon)
		{
			auto meshrenderer = dragon->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->RenderForward(*coreRef);
		}
	}
}

void LoginScene::RenderSceneShadow()
{
}

void LoginScene::RenderSceneEffects()
{
}

int LoginScene::GetSceneWidth() const
{
	return 0;
}

void LoginScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::ServerSquare);
	}
}
