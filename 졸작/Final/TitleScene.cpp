#include "pch.h"
#include "TitleScene.h"
#include "Material.h"
#include "Engine.h"
#include "SoundManager.h"

void TitleScene::Release()
{
	SOUND_MANAGER->StopBGM(0.5f);

	OutputDebugStringA("TitleScene Data has been deleted!! \n----------------------------------------\n");
}

const char* TitleScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/LoginBGM.mp3";
}

void TitleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nTitleScene Data has been created!! \n");

	// TODO
	// 여기에서, 모든 캐릭터와 모든 몬스터의 MESH 미리 캐싱
	
	PreloadCommonTextures();
	PreloadAllCharactersMeshes();
	SOUND_MANAGER->PreloadEverySFX();

	OutputDebugStringA("Data cached created!!\n");
}

void TitleScene::PreloadCommonTextures()
{
	ID3D12Device* device = coreRef->GetDevice();
	ID3D12GraphicsCommandList* cmdList = coreRef->GetGraphicsCmdList();

	const wchar_t* skyboxNames[] = { L"skybox", L"skybox1", L"skybox2", L"skybox3" };
	for (auto name : skyboxNames)
	{
		wstring base = wstring(L"../Assets/Skybox/") + name;
		Material::RegisterCubeMap(device, cmdList, base + L".dds");
		Material::RegisterCubeMap(device, cmdList, base + L"_irradiance.dds");
		Material::RegisterCubeMap(device, cmdList, base + L"_radiance.dds");
	}
	Material::RegisterTexture(device, cmdList, L"../Assets/Skybox/brdf_lut.png");
	Material::RegisterTexture(device, cmdList, L"../Assets/FBXModel/CastleMap/textures/rdiffuse.png");
	Material::RegisterTexture(device, cmdList, L"../Assets/FBXModel/CastleMap/textures/rnormal.png");
	Material::RegisterTexture(device, cmdList, L"../Assets/FBXModel/CastleMap/textures/grass.png");
	Material::RegisterTexture(device, cmdList, L"../Assets/FBXModel/CastleMap/textures/terrainTexture.png");
}

void TitleScene::PreloadAllCharactersMeshes()
{
	const wchar_t* paths[] = {
	  L"../Assets/FBXModel/Knight/knight6",
	  L"../Assets/FBXModel/Lancer/lancer",
	  L"../Assets/FBXModel/Paladin/paladin",
	  L"../Assets/FBXModel/Boss/boss",
	  L"../Assets/FBXModel/Monster/Imp/monster_Imp",
	  L"../Assets/FBXModel/Monster/DemonStriker/monster_DemonStriker",
	  L"../Assets/FBXModel/Monster/DemonExecutioner/monster_DemonExecutioner",
	  L"../Assets/FBXModel/Monster/BigDemonWarrior/monster_BigDemonWarrior",
	  L"../Assets/FBXModel/Monster/Tank/monster_Tank",
	  L"../Assets/FBXModel/Potion/potion",
	};

	vector<shared_ptr<GameObject>> objs;
	objs.reserve(_countof(paths));

	for (auto p : paths) {
		auto obj = make_shared<GameObject>();
		auto m = obj->AddComponent<Mesh>();
		m->SetMesh(*coreRef, p);
		objs.push_back(obj);
	}

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	for (auto& obj : objs)
		obj->GetComponent<Mesh>()->ReleaseUploadBuffers();
}
