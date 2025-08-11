#include "pch.h"
#include "TestScene.h"
#include "Camera.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Input.h"
#include "SceneManager.h"
#include "UploadBuffer.h"
#include "Material.h"

TestScene::~TestScene() = default;

void TestScene::Release()
{

}

void TestScene::Reset()
{
	knightTemplate.reset();
	knightMatrix.clear();
	instanceBuffer.reset();

	Material::ResetStartIndex();
	OutputDebugStringA("TestScene Data has been deleted!! \n----------------------------------------\n");
}

const float* TestScene::GetBackgroundColor()
{
	return Colors::LightBlue;
}

void TestScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	OutputDebugStringA("----------------------------------------\nTestScene Data has been created!! \n");
    GET(Camera).Initialize();

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
}

void TestScene::UpdateScene(const float deltaTime)
{
	GET(Camera).Update(deltaTime);

	knightTemplate->Update(deltaTime);

	if (GET(Input).GetKeyDown(VK_TAB))
		GET(SceneManager).RequestSceneChange(SceneType::Login);
}

void TestScene::RenderScene()
{
	if (knightTemplate)
	{
		auto meshRenderer = knightTemplate->GetComponent<MeshRenderer>();
		if (meshRenderer)
			meshRenderer->RenderInstanced(INSTANCE_COUNT, instanceBuffer.get());
	}
}

int TestScene::GetSceneWidth() const
{
	return 0;
}
