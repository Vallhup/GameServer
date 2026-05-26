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
		  .lut = { .lutIndex = 14, .saturation = 1.0f },
		  .fog = { .density = 0.03f, .maxSteps = 32, .maxDistance = 110.0f,
					  .jitterStrength = 1.0f, .groundHeight = 48.0f, .lightIntensity = 1.0f },
		  .skybox = { .tintColor = { 1.0f, 1.0f, 1.0f }, .saturation = 1.0f },
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
	light->Spawn({ 335.237946f, 77.0f, 590.663147f });
	AddGameObject(beacon);

	beaconLight = light;   
#pragma endregion

	EFFECT_MANAGER->PreLoad(L"Benediction");
	EFFECT_MANAGER->PreLoad(L"Atmosphere");
}

void FirstBattleScene::InitializeSceneMonsters()
{
	const XMFLOAT3 monsterSpawn = { 22.f, SampleHeightAt(22.f, 22.f), 22.f };
	CreateMonsters(MonsterType::Imp, monsterSpawn, 9);
	CreateMonsters(MonsterType::DemonStriker, monsterSpawn, 1);
	CreateMonsters(MonsterType::DemonExecutioner, monsterSpawn, 1);
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
		if (pos.x < 167.0f && pos.x > 162.0f && pos.y > 49.3f && pos.z < 644.0f)
		{
			auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Village);
			if (controller)
				controller->ShowMapName();
		}
	}

	if (myPlayer && cineState == BeaconCine::None)
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
				cineState = BeaconCine::FadeOut;
				cineTimer = 0.0f;
				controller->HideHudForCinematic();   
				INPUT.SetBlocked(true);
				for (auto& batch : instancingBatches)
					batch->SetCinematicMode(true);   

				for (const auto& [mid, mtype] : activeMonsterTypes)
					if (auto it = activeCharacters.find(mid); it != activeCharacters.end())
						it->second->SetId(-1);
				if (auto* fade = ENGINE.GetUIManager()->GetScreenFade())
					fade->FadeOut(CINE_FADE_DUR);
			}
		}
	}

	const bool cineActive = (cineState != BeaconCine::None);

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

void FirstBattleScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_CAPITAL))
	{
		// TODO: 서버 검증 이후 LoadingScene 입장하도록 변경 예정
		//if (sManagerRef)
		//	sManagerRef->RequestLoadingScene(SceneType::Castle);

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

const char* FirstBattleScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/VillageBGM.mp3";
}

float FirstBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

void FirstBattleScene::UpdateCinematicCamera(const XMFLOAT3& look)
{
	if (!cam) return;

	float bx = CINE_CAM_EYE_X - CINE_LOOK_X;
	float bz = CINE_CAM_EYE_Z - CINE_LOOK_Z;
	const float bl = sqrtf(bx * bx + bz * bz);
	if (bl > 0.0001f) { bx /= bl; bz /= bl; }

	const XMFLOAT3 eye{
		CINE_CAM_EYE_X + bx * CINE_CAM_BACK,
		beaconCinePos.y + CINE_CAM_Y_ABOVE,
		CINE_CAM_EYE_Z + bz * CINE_CAM_BACK };
	cam->SetCinematicView(*coreRef, eye, look);
}

void FirstBattleScene::ScatterAtmosphere()
{
	constexpr float CENTER_X = 250.0f;       
	constexpr float CENTER_Z = 600.0f;       
	constexpr float SPAN_X   = 300.0f;       
	constexpr float SPAN_Z   = 200.0f;       
	constexpr int   NX = 11;                 
	constexpr int   NZ = 8;                  
	constexpr float LAYER_OFFSETS[] = { 30.0f };

	const float startX = CENTER_X - SPAN_X * 0.5f;
	const float startZ = CENTER_Z - SPAN_Z * 0.5f;
	const float stepX  = SPAN_X / (NX - 1);
	const float stepZ  = SPAN_Z / (NZ - 1);

	for (int i = 0; i < NX; ++i)
		for (int j = 0; j < NZ; ++j)
		{
			const float x = startX + i * stepX;
			const float z = startZ + j * stepZ;
			const float ground = SampleHeightAt(x, z);
			for (float dy : LAYER_OFFSETS)
				atmosphereHandles.push_back(EFFECT_MANAGER->Play(L"Atmosphere", { x, ground + dy, z }));
		}
}

void FirstBattleScene::CaptureBrightenBase()
{
	if (!skyBox) return;
	cineSunBase = skyBox->GetSun().intensity;
	auto& sc = skyBox->GetConstants();
	cineSkySatBase = sc.skySaturation;
	cineSkyExpBase = sc.skyExposure;
}

void FirstBattleScene::ApplyBrighten(float t)
{
	if (!skyBox) return;

	skyBox->GetSun().intensity = cineSunBase + (cineSunBase * CINE_SUN_MULT - cineSunBase) * t;
	coreRef->GetLightMgr()->UpdateLights();

	auto& sc = skyBox->GetConstants();
	sc.skySaturation = cineSkySatBase + (cineSkySatBase * CINE_SKY_SAT_MULT - cineSkySatBase) * t;
	sc.skyExposure = cineSkyExpBase + (cineSkyExpBase * CINE_SKY_EXP_MULT - cineSkyExpBase) * t;
	skyBox->UpdateConstants();
}

void FirstBattleScene::UpdateBeaconCinematic(float deltaTime)
{
	auto* fade = ENGINE.GetUIManager()->GetScreenFade();
	cineTimer += deltaTime;

	switch (cineState)
	{
	case BeaconCine::FadeOut:
		if (fade && fade->IsBlack())
		{
			cineState = BeaconCine::Rising;
			cineTimer = 0.0f;
			beaconCinePos = { 335.237946f, 77.0f, 590.663147f };
			beaconCineSize = CINE_BEACON_BASE_SIZE;
			if (beaconLight) { beaconLight->SetPosition(beaconCinePos); beaconLight->SetSize(beaconCineSize); }
			UpdateCinematicCamera(beaconCinePos);   
			fade->FadeIn(CINE_FADE_DUR);
		}
		break;

	case BeaconCine::Rising:
	{
		const float t = min(cineTimer / CINE_RISE_DUR, 1.0f);
		beaconCinePos.y = 77.0f + CINE_RISE_HEIGHT * t;     
		if (beaconLight) beaconLight->SetPosition(beaconCinePos);
		UpdateCinematicCamera(beaconCinePos);   
		if (t >= 1.0f) { cineState = BeaconCine::Growing; cineTimer = 0.0f; }
		break;
	}

	case BeaconCine::Growing:
	{
		const float t = min(cineTimer / CINE_GROW_DUR, 1.0f);
		beaconCineSize = CINE_BEACON_BASE_SIZE + (CINE_BEACON_MAX_SIZE - CINE_BEACON_BASE_SIZE) * t;
		if (beaconLight) beaconLight->SetSize(beaconCineSize);
		UpdateCinematicCamera(beaconCinePos);   
		if (t >= 1.0f)
		{
			EFFECT_MANAGER->Play(L"Benediction", beaconCinePos);
			if (beaconLight) beaconLight->Stop();
			ScatterAtmosphere();
			CaptureBrightenBase();
			cineState = BeaconCine::Showcase;
			cineTimer = 0.0f;
		}
		break;
	}

	case BeaconCine::Showcase:
	{
		const float t = min(cineTimer / CINE_BRIGHTEN_DUR, 1.0f);
		const XMFLOAT3 look{
			beaconCinePos.x + (CINE_LOOK_X - beaconCinePos.x) * t,
			beaconCinePos.y + (CINE_LOOK_Y - beaconCinePos.y) * t,
			beaconCinePos.z + (CINE_LOOK_Z - beaconCinePos.z) * t };
		UpdateCinematicCamera(look);
		ApplyBrighten(t);
		if (cineTimer >= CINE_BRIGHTEN_DUR + CINE_SHOWCASE_HOLD_DUR)
		{
			if (fade)
			{
				const auto handles = atmosphereHandles;   
				atmosphereHandles.clear();
				fade->SetOnFadedOut([handles]() {
					for (int h : handles)          
						EFFECT_MANAGER->Stop(h);

					auto& tr = ENGINE.GetWorldTransitionController();
					const uint32_t rid = tr.CreateRequestId();
					if (tr.BeginRequest(rid))
						if (!NETWORK_MANAGER->SendWorldTransitionRequestPacket(rid))
							tr.Reset();
				});
				fade->FadeOut(CINE_FADE_DUR);
			}
			cineState = BeaconCine::Done;
		}
		break;
	}

	case BeaconCine::Done:
	default:
		break;
	}
}
