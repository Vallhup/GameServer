#include "pch.h"
#include "FirstBattleScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Engine.h"
#include "Terrain.h"
#include "Water.h"
#include "SkyBox.h"
#include "LightManager.h"
#include "ShadowMappingManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "SoundManager.h"
#include "EffectManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "EffectComponent.h"
#include "FlameComponent.h"
#include "BeaconLightComponent.h"

void FirstBattleScene::Release()
{
	instancingBatches.clear();
	characterPools.clear();
	monsterPools.clear();
	activeCharacters.clear();
	activeMonsterTypes.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	INPUT.SetBlocked(false);   
	SOUND_MANAGER->StopBGM(1.0f);

	OutputDebugStringA("FirstBattleScene Data has been deleted!! \n----------------------------------------\n");
}

SceneSettings FirstBattleScene::GetSceneSettings() const
{
	return {
		  .light = { .sunIntensity = 1.0f },
		  .lut = { .lutIndex = 14, .saturation = 1.5f },
		  .fog = { .density = 0.03f, .scattering = 1.4f, .maxSteps = 128, .maxDistance = 50.0f,
					  .jitterStrength = 1.0f, .groundHeight = 48.0f, .lightColor = { 0.7764f, 0.6313f, 0.6313f }, .lightIntensity = 1.0f},
		  .skybox = { .tintColor = { 0.9058f, 0.7411f, 0.7411f }, .exposure = 0.6f, .saturation = 1.0f },
		  .shadow = {.shadowAmbientMin = 1.0f, .shadowFloor = 0.0f },
	};
}

void FirstBattleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nFirstBattleScene Data has been created!! \n");

	cam->SetCursor(false);

	CreateCharacterPool(CharacterType::Knight);
	CreateCharacterPool(CharacterType::Lancer);
	CreateCharacterPool(CharacterType::Paladin);

	InitializeSceneEnvironments();
	InitializeSceneMonsters();

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	OutputDebugStringA("FirstBattleScene initialized!\n");
}

void FirstBattleScene::InitializeSceneEnvironments()
{
	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"skybox1");
	cineSkyBox = skyBox.get();
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());
	coreRef->GetLightMgr()->SetSkyBox(skyBox.get());
	coreRef->GetShadowMgr()->SetLightMgr(coreRef->GetLightMgr());
	coreRef->GetLightMgr()->LoadSceneLights(L"../Assets/FBXModel/VillageMap/VillageLightData.txt", true);
	coreRef->GetLightMgr()->UpdateLights();

#pragma region Intialize Candles
	auto* lm = coreRef->GetLightMgr();
	const LightData* lights = lm->GetLights();
	int lcount = lm->GetDeferredLightData().lightCount;

	auto flameObject = make_shared<GameObject>();
	flameObject->SetId(-1);
	auto flame = flameObject->AddComponent<FlameComponent>();
	flame->Initialize(coreRef->GetDevice(), 96);
	flame->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/T_candleflame.png");
	flame->SetParticleSize(0.35f);
	for (int i = 1; i < lcount; ++i)
		flame->Spawn(lights[i].position);
	AddGameObject(flameObject);
#pragma endregion

#pragma region Initialize Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"../Assets/FBXModel/VillageMap/ground", L"../Assets/FBXModel/VillageMap/villageTerrain.raw", 513, 1023.0f, 159.4766f, 2.0f);
	terrain->LoadSplatmap(*coreRef, L"../Assets/FBXModel/VillageMap/terrainAtlas.bin", L"../Assets/FBXModel/VillageMap/textures/");
	cam->SetTerrain(terrain.get());
#pragma endregion

#pragma region Initialize Ocean Floor
	oceanFloor = make_shared<Terrain>();
	oceanFloor->Initialize(*coreRef, L"VillageMap/textures/OceanFloor", L"../Assets/FBXModel/VillageMap/oceanFloorTerrain.raw", 513, 1946.701f, 170.3121f, 2.0f);
	oceanFloor->SetPosition(-903.851200f, 32.799990f, 160.528700f);
#pragma endregion

#pragma region Initialize Water
	water = make_shared<Water>();
	water->Initialize(*coreRef);
	water->SetPosition(144.0472f, 46.79999f, 939.9999f);
	water->SetScale(1500.0f, 1.0f, 2546.25f);
#pragma endregion

#pragma region Initialize BeaconLight
	auto beacon = make_shared<GameObject>();
	auto light = beacon->AddComponent<BeaconLightComponent>();
	light->Initialize(coreRef->GetDevice(), 2);
	light->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(),
		L"../Assets/Effects/Textures/particle2.png");
	light->SetColor({ 2.854f, 2.439f, 1.5f, 1.0f });
	light->SetSize(2.0f);
	AddGameObject(beacon);

	beaconLight = light;
	beaconSpawnPos = { 335.237946f, 77.0f, 590.663147f };	
#pragma endregion

	EFFECT_MANAGER->PreLoad(L"Benediction");
	EFFECT_MANAGER->PreLoad(L"Atmosphere");
}

void FirstBattleScene::InitializeSceneMonsters()
{
	const XMFLOAT3 monsterSpawn = { 22.f, SampleHeightAt(22.f, 22.f), 22.f };
	CreateMonsters(MonsterType::Imp, monsterSpawn, 12);
	CreateMonsters(MonsterType::DemonStriker, monsterSpawn, 3);
	CreateMonsters(MonsterType::DemonExecutioner, monsterSpawn, 3);
	CreateMonsters(MonsterType::BigDemonWarrior, monsterSpawn, 1);
}

void FirstBattleScene::UpdateScene(const float deltaTime)
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
		if (pos.x < 181.913f && pos.x > 171.423f && pos.z > 617.894f && pos.z < 628.343f)
		{
			auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Village);
			if (controller)
				controller->ShowMapName();
		}
	}

	if (myPlayer && !IsCinematicActive() && beaconLight && beaconLight->IsAlive())
	{
		constexpr float BEACON_CX = 335.237946f;
		constexpr float BEACON_CZ = 590.663147f;
		constexpr float INTERACT_RADIUS = 2.0f;
		constexpr float UI_HEIGHT = 76.7f;

		const XMFLOAT3& pos = myPlayer->GetComponent<Transform>()->GetPosition();
		const float dx = pos.x - BEACON_CX;
		const float dz = pos.z - BEACON_CZ;
		const float distSq = dx * dx + dz * dz;

		auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Village);
		if (controller)
		{
			if (distSq <= INTERACT_RADIUS * INTERACT_RADIUS)
			{
				const XMFLOAT3 anchor{ BEACON_CX, UI_HEIGHT, BEACON_CZ };
				controller->SetInteractPrompt(true, anchor);
			}
			else
			{
				controller->SetInteractPrompt(false, {});
			}

			if (controller->ConsumeBeaconConfirmed())
			{
				controller->SetInteractPrompt(false, {});
				NETWORK_MANAGER->SendBeaconCinematicStartRequest();
			}
		}
	}

	const bool cineActive = IsCinematicActive();

	for (const auto& obj : gameObjects)
	{
		if (cineActive && myPlayer && obj.get() == myPlayer.get())
			continue;
		if (!obj->IsStatic())
			obj->Update(deltaTime);
	}

	if (water)
		water->Update(deltaTime);

	if (cineActive)
		UpdateBeaconCinematic(deltaTime);

	if (cam && !cineActive)
		cam->Update(*coreRef, deltaTime, gameObjects, instancingBatches, myPlayer);

	BoundingFrustum frustum = cam->GetViewFrustum();
	XMFLOAT3 camPos = cam->GetPosition();
	XMVECTOR camPosVec = XMLoadFloat3(&camPos);
	XMFLOAT3 playerPos = cam->GetTargetPosition();
	XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);

	for (auto& batch : instancingBatches)
		batch->Update(frustum, camPosVec, playerPosVec);
}

void FirstBattleScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_CAPITAL))
	{
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

void FirstBattleScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());

	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

	if (oceanFloor)
		renderer->RenderTerrain(*coreRef, oceanFloor.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}
}

void FirstBattleScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());

	if (water)
		renderer->RenderWater(*coreRef, water.get());
}

void FirstBattleScene::RenderSceneShadowStatic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowStatic(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadowStatic(*coreRef, renderer);
	}
}

void FirstBattleScene::RenderSceneShadowDynamic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowDynamic(*coreRef, gameObjects);
}

void FirstBattleScene::RenderSceneEffects()
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

const char* FirstBattleScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/VillageBGM.mp3";
}

const char* FirstBattleScene::GetBossBGMPath() const
{
	return "../Assets/Music/BGM/VillageBossBGM.mp3";
}

float FirstBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

const BeaconCinematicConfig& FirstBattleScene::GetCinematicConfig() const
{
	static const BeaconCinematicConfig cfg{
		.riseStart       = { 335.237946f, 77.0f, 590.663147f },
		.riseEnd         = { 335.237946f, 97.0f, 590.663147f },
		.riseVerticalRatio = 1.0f,
		.fadeDur         = 1.0f,
		.riseDur         = 6.0f,
		.growDur         = 1.0f,
		.brightenDur     = 2.5f,
		.showcaseHoldDur = 8.0f,
		.beaconBaseSize  = 2.0f,
		.beaconMaxSize   = 60.0f,
		.camEyeXZ        = { 352.913971f, 586.207336f },
		.camYAbove       = 20.0f,
		.camBack         = 30.0f,
		.lookTarget      = { 243.137360f, 58.121223f, 606.244629f },
		.sunMult         = 6.0f,
		.skySatMult      = 2.0f,
		.skyExpMult      = 1.5f,
		.scatterCenter   = { 250.0f, 600.0f },
		.scatterSpan     = { 300.0f, 200.0f },
		.scatterNX       = 11,
		.scatterNZ       = 8,
		.scatterLayerY   = 30.0f,
		.burstEffect       = L"Benediction",
		.atmosphereEffect  = L"Atmosphere",
	};
	return cfg;
}
