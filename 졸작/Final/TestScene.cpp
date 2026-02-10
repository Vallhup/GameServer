#include "pch.h"
#include "TestScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Input.h"
#include "Material.h"
#include "Animator.h"

TestScene::~TestScene() = default;

void TestScene::Release()
{

}

void TestScene::Reset()
{
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
		auto knight = make_shared<MainCharacter>();
		auto mesh = knight->AddComponent<Mesh>();
		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		mesh->ReleaseUploadBuffers();
	}
}

void TestScene::UpdateScene(const float deltaTime)
{	
}

void TestScene::RenderSceneDeferred()
{
}

void TestScene::RenderSceneForward()
{
}

void TestScene::RenderSceneShadow()
{
}

void TestScene::RenderSceneEffects()
{
}

void TestScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Login);
	}
}
