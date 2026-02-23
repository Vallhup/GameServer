#include "pch.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Input.h"
#include "Material.h"
#include "Animator.h"

TitleScene::~TitleScene() = default;

void TitleScene::Release()
{

}

void TitleScene::Reset()
{
	Material::ReleaseUploadBuffers();
	OutputDebugStringA("TitleScene Data has been deleted!! \n----------------------------------------\n");
}

void TitleScene::InitializeSceneObjectPools()
{
}

void TitleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nTitleScene Data has been created!! \n");

	
	auto knight = make_shared<MainCharacter>();
	auto mesh = knight->AddComponent<Mesh>();
	mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");
	
	auto dragon = make_shared<GameObject>();
	auto mesh2 = dragon->AddComponent<Mesh>();
	mesh2->SetMesh(*coreRef, L"../Assets/FBXModel/Dragon/Dragon");

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	mesh->ReleaseUploadBuffers();
	mesh2->ReleaseUploadBuffers();

	OutputDebugStringA("Dragon created!!\n");
	
}

void TitleScene::UpdateScene(const float deltaTime)
{	
}

void TitleScene::RenderSceneDeferred()
{
}

void TitleScene::RenderSceneForward()
{
}

void TitleScene::RenderSceneShadow()
{
}

void TitleScene::RenderSceneEffects()
{
}

void TitleScene::RequestSceneChange()
{
	/*if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Select);
	}*/
}
