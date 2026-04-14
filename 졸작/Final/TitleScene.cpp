#include "pch.h"
#include "TitleScene.h"
#include "Material.h"

void TitleScene::Release()
{

}

void TitleScene::Reset()
{
	OutputDebugStringA("TitleScene Data has been deleted!! \n----------------------------------------\n");
}

void TitleScene::InitializeSceneObjectPools()
{
}

void TitleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nTitleScene Data has been created!! \n");

	// TODO
	// 여기에서, 모든 캐릭터와 모든 몬스터의 MESH 미리 캐싱
	
	auto knight = make_shared<GameObject>();
	auto mesh = knight->AddComponent<Mesh>();
	mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");

	auto lancer = make_shared<GameObject>();
	auto mesh1 = lancer->AddComponent<Mesh>();
	mesh1->SetMesh(*coreRef, L"../Assets/FBXModel/Lancer/lancer");

	auto paladin = make_shared<GameObject>();
	auto mesh2 = paladin->AddComponent<Mesh>();
	mesh2->SetMesh(*coreRef, L"../Assets/FBXModel/Paladin/paladin");

	auto boss = make_shared<GameObject>();
	auto mesh3 = boss->AddComponent<Mesh>();
	mesh3->SetMesh(*coreRef, L"../Assets/FBXModel/Boss/boss");

	auto demonStriker = make_shared<GameObject>();
	auto mesh4 = demonStriker->AddComponent<Mesh>();
	mesh4->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/DemonStriker/monster_DemonStriker");

	auto imp = make_shared<GameObject>();
	auto mesh5 = imp->AddComponent<Mesh>();
	mesh5->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/Imp/monster_Imp");

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	mesh->ReleaseUploadBuffers();
	mesh1->ReleaseUploadBuffers();
	mesh2->ReleaseUploadBuffers();
	mesh3->ReleaseUploadBuffers();
	mesh4->ReleaseUploadBuffers();
	mesh5->ReleaseUploadBuffers();

	OutputDebugStringA("Data cached created!!\n");
	
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
}
