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
	knightTemplate.reset();
	knightMatrix.clear();
	instanceBuffer.reset();

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
		transform->SetPosition(0.f, 0.f, 0.f);
		transform->SetRotation(-1.57f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		meshrenderer->ReleaseUploadBuffers();

		knight->SetCamera(cam.get());
	}

	/*{
		strut = make_shared<GameObject>();
		auto meshrenderer = strut->AddComponent<MeshRenderer>();
		auto transform = strut->AddComponent<Transform>();
		auto animator = strut->AddComponent<Animator>();
		meshrenderer->SetMesh(*coreRef, L"../FBXOutput/Strut Walking");
		transform->SetPosition(0.f, 0.f, -0.5f);
		transform->SetRotation(-1.57f, 0.f, 0.f);	
		transform->SetScale(0.01f, 0.01f, 0.01f);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		meshrenderer->ReleaseUploadBuffers();
	}*/

	/*{
		knightTemplate = make_shared<GameObject>();
		auto meshRenderer = knightTemplate->AddComponent<MeshRenderer>();
		meshRenderer->SetMesh(L"../FBXOutput/Strut Walking");

		auto transform = knightTemplate->AddComponent<Transform>();


		knightMatrix.resize(INSTANCE_COUNT);
		for (int i = 0; i < 100; ++i)
		{
			for (int j = 0; j < 100; ++j)
			{
				XMMATRIX S = XMMatrixScaling(0.01f, 0.01f, 0.01f);
				XMMATRIX R = XMMatrixRotationRollPitchYaw(-1.57f, 0.f, 0.f);
				XMMATRIX T = XMMatrixTranslation(-5.0f + (i * 0.5f), 0.f, -5.0f + (j * 0.5f));

				knightMatrix[j + (i * 100)] = XMMatrixTranspose(S * R * T);
			}
		}

		instanceBuffer = make_unique<UploadBuffer>();
		instanceBuffer->Initialize(device, sizeof(XMMATRIX) * INSTANCE_COUNT);
		instanceBuffer->CopyData(knightMatrix.data(), sizeof(XMMATRIX) * INSTANCE_COUNT);

		OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
		GET(DX12Graphics).FlushCommandQueue();
		GET(DX12Graphics).ResetCommandQueue();

		meshRenderer->ReleaseUploadBuffers();
		OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");
	}*/
}

void TestScene::UpdateScene(const float deltaTime)
{
	{
		knight->Update(deltaTime);
	}

	/*{
		knightTemplate->Update(deltaTime);
	}*/
}

void TestScene::RenderScene()
{
	{
		if (knight)
		{
			auto meshrenderer = knight->GetComponent<MeshRenderer>();
			if (meshrenderer)
				meshrenderer->Render(*coreRef);
		}
	}

	/*{
		if (knightTemplate)
		{
			auto meshRenderer = knightTemplate->GetComponent<MeshRenderer>();
			if (meshRenderer)
				meshRenderer->RenderInstanced(INSTANCE_COUNT, instanceBuffer.get());
		}
	}*/
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
