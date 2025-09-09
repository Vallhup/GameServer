#include "pch.h"
#include "ServerSquareScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Camera.h"
#include "MainCharacter.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Animator.h"

ServerSquareScene::~ServerSquareScene() = default;

void ServerSquareScene::Release()
{
	knight.reset();
}

void ServerSquareScene::Reset()
{
	Material::Cleanup();
	OutputDebugStringA("ServerSquareScene Data has been deleted!! \n----------------------------------------\n");
}

const float* ServerSquareScene::GetBackgroundColor()
{
	return Colors::Pink;
}

void ServerSquareScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nServerSquareScene Data has been created!! \n");

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

void ServerSquareScene::UpdateScene(const float deltaTime)
{
	{
		knight->Update(deltaTime);
	}
}

void ServerSquareScene::RenderSceneDeferred()
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

void ServerSquareScene::RenderSceneForward()
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

void ServerSquareScene::RenderSceneEffects()
{
}

int ServerSquareScene::GetSceneWidth() const
{
	return 0;
}

void ServerSquareScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::MainGame);
	}
}
