#include "pch.h"
#include "SecondBattleScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Engine.h"
#include "Terrain.h"
#include "SkyBox.h"
#include "LightManager.h"
#include "ShadowMappingManager.h"
#include "Water.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "SoundManager.h"
#include "EffectManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "EffectComponent.h"

void SecondBattleScene::Release()
{
	instancingBatches.clear();
	characterPools.clear();
	monsterPools.clear();
	activeCharacters.clear();
	activeMonsterTypes.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	OutputDebugStringA("SecondBattleScene Data has been deleted!! \n----------------------------------------\n");
}

SceneSettings SecondBattleScene::GetSceneSettings() const
{
	return {
		  .light = { .sunIntensity = 0.2f },
		  .lut = { .lutIndex = 14, .saturation = 1.0f },
		  .fog = { .density = 0.02f, .maxSteps = 32, .maxDistance = 90.0f,
					  .jitterStrength = 1.0f, .groundHeight = 66.0f, .lightIntensity = 0.8f },
		  .skybox = { .tintColor = { 1.0f, 1.0f, 1.0f }, .exposure = 0.6f, .saturation = 1.0f },
	};
}

void SecondBattleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nSecondBattleScene Data has been created!! \n");

	cam->SetCursor(false);

	CreateCharacterPool(CharacterType::Knight);
	CreateCharacterPool(CharacterType::Lancer);
	CreateCharacterPool(CharacterType::Paladin);

	InitializeSceneEnvironments();
	InitializeSceneMonsters();

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	OutputDebugStringA("SecondBattleScene initialized!\n");
}

void SecondBattleScene::InitializeSceneEnvironments()
{
	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"skybox2");
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());
	coreRef->GetLightMgr()->SetSkyBox(skyBox.get());
	coreRef->GetShadowMgr()->SetLightMgr(coreRef->GetLightMgr());
	coreRef->GetLightMgr()->LoadSceneLights(L"../Assets/FBXModel/CastleMap/CastleLightData.txt", true);
	coreRef->GetLightMgr()->UpdateLights();

#pragma region Initialize Castle Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"CastleMap/textures/CastleFloor", L"../Assets/FBXModel/CastleMap/castleTerrain.raw", 513, 650.2402f, 79.28662f, 8.0f);
	terrain->LoadSplatmap(*coreRef, L"../Assets/FBXModel/CastleMap/terrainAtlas.bin", L"../Assets/FBXModel/CastleMap/textures/");
#pragma endregion

#pragma region Initialize Water
	water = make_shared<Water>();
	water->Initialize(*coreRef);
	water->SetPosition(378.874207f, 53.3f, 367.952576f);
	water->SetScale(170.0f, 1.0f, 170.0f);
	XMFLOAT4 color = { 0.0f, 0.6f, 0.85f, 0.7f };
	water->SetColor(color);
#pragma endregion
}

void SecondBattleScene::InitializeSceneMonsters()
{
	const XMFLOAT3 monsterSpawn = { 22.f, SampleHeightAt(22.f, 22.f), 22.f };
	CreateMonsters(MonsterType::Imp, monsterSpawn, 6);
	CreateMonsters(MonsterType::DemonStriker, monsterSpawn, 1);
	CreateMonsters(MonsterType::DemonExecutioner, monsterSpawn, 1);
	CreateMonsters(MonsterType::Tank, monsterSpawn, 1);
}

void SecondBattleScene::UpdateScene(const float deltaTime)
{
	if (myPlayer)
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());

		if (INPUT.GetKeyDown('0'))
		{
			XMFLOAT3 pos = transform->GetPosition();
			OutputDebugStringA(("MyPlayer Pos: " + to_string(pos.x) + ", " + to_string(pos.y) + ", " + to_string(pos.z) + "\n").c_str());
		}
	}

	if (myPlayer)
	{
		auto& pos = myPlayer->GetComponent<Transform>()->GetPosition();
		if (pos.x < 325.8f && pos.x > 322.5f && pos.z > 220.1f && pos.z < 221.2f)
		{
			auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Castle);
			if (controller)
				controller->ShowMapName();
		}
	}

	for (const auto& obj : gameObjects)
	{
		if (!obj->IsStatic())
			obj->Update(deltaTime);
	}

	if (water)
		water->Update(deltaTime);

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, instancingBatches, myPlayer);

	BoundingFrustum frustum = cam->GetViewFrustum();
	XMFLOAT3 camPos = cam->GetPosition();
	XMVECTOR camPosVec = XMLoadFloat3(&camPos);
	XMFLOAT3 playerPos = cam->GetTargetPosition();
	XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);

	for (auto& batch : instancingBatches)
		batch->Update(frustum, camPosVec, playerPosVec);
}

void SecondBattleScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());

	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}
}

void SecondBattleScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());

	if (water)
		renderer->RenderWater(*coreRef, water.get());
}

void SecondBattleScene::RenderSceneShadowStatic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowStatic(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadowStatic(*coreRef, renderer);
	}
}

void SecondBattleScene::RenderSceneShadowDynamic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowDynamic(*coreRef, gameObjects);
}

void SecondBattleScene::RenderSceneEffects()
{
	if (cam)
	{
		const XMFLOAT3 camPos = cam->GetPosition();

		for (const auto& obj : gameObjects)
		{
			for (auto& [type, comp] : obj->GetComponents())
			{
				if (auto effect = dynamic_cast<EffectComponent*>(comp.get()))
					effect->Render(*coreRef, camPos);
			}
		}

		EFFECT_MANAGER->Render(*coreRef, cam.get());
	}
}

void SecondBattleScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_CAPITAL))
	{
		// TODO: 서버 검증 이후 LoadingScene 입장하도록 변경 예정
		//if (sManagerRef)
		//	sManagerRef->RequestLoadingScene(SceneType::Village);

		auto& transition = ENGINE.GetWorldTransitionController();
		const uint32_t requestId = transition.CreateRequestId();

		if (transition.BeginRequest(requestId))
		{
			if (!NETWORK_MANAGER->SendWorldTransitionRequestPacket(requestId))
			{
				transition.Reset();
			}
		}
	}
}

float SecondBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}
