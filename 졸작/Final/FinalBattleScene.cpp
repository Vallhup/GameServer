#include "pch.h"
#include "FinalBattleScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Engine.h"
#include "SkyBox.h"
#include "LightManager.h"
#include "ShadowMappingManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "EffectManager.h"
#include "EffectComponent.h"
#include "FlameComponent.h"
#include "Animator.h"
#include "SoundManager.h"

void FinalBattleScene::Release()
{
	instancingBatches.clear();
	characterPools.clear();
	monsterPools.clear();
	activeCharacters.clear();
	activeMonsterTypes.clear();
	gimmickPool.clear();
	activeGimmicks.clear();
	activeZoneBarriers.clear();
	activeBreakerShields.clear();
	activeBuffEffects.clear();
	activeTitleEffects.clear();
	titleIdByOwner.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	if (auto* controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Final))
		controller->ResetEndingOverlays();

	SOUND_MANAGER->StopBGM(1.0f);

	OutputDebugStringA("FinalBattleScene Data has been deleted!! \n----------------------------------------\n");
}

SceneSettings FinalBattleScene::GetSceneSettings() const
{
	return {
		  .light = { .sunDirection = { 0.0f, -0.75f, -1.0f }, .sunIntensity = 0.0f},
		  .lut = { .lutIndex = 104, .saturation = 1.05f },
		  .fog = { .density = 0.0f, .maxSteps = 32, .maxDistance = 110.0f,
					  .jitterStrength = 1.0f, .groundHeight = 2.0f, .lightIntensity = 0.0f },
		  .skybox = { .tintColor = { 1.0f, 1.0f, 1.0f }, .saturation = 1.0f },
	};
}

void FinalBattleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nFinalBattleScene Data has been created!! \n");

	cam->SetCursor(false);

	CreateCharacterPool(CharacterType::Knight);
	CreateCharacterPool(CharacterType::Lancer);
	CreateCharacterPool(CharacterType::Paladin);

	InitializeSceneEnvironments();
	InitializeSceneMonsters();

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	UI_MANAGER->PlayVideo(L"../Assets/Movie/FinalBoss.mp4");

	OutputDebugStringA("FinalBattleScene initialized!\n");
}

void FinalBattleScene::InitializeSceneEnvironments()
{
	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"skybox3");
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());
	coreRef->GetLightMgr()->SetSkyBox(skyBox.get());
	coreRef->GetShadowMgr()->SetLightMgr(coreRef->GetLightMgr());
	coreRef->GetLightMgr()->LoadSceneLights(L"../Assets/FBXModel/GothicMap/FinalMapLightData.txt", false);
	coreRef->GetLightMgr()->UpdateLights();

#pragma region Intialize Candles
	auto* lm = coreRef->GetLightMgr();
	vector<XMFLOAT3> positions = lm->LoadCandlePositions(L"../Assets/FBXModel/GothicMap/GothicCandles.txt");
	auto flameObject = make_shared<GameObject>();
	flameObject->SetId(-1);
	auto flame = flameObject->AddComponent<FlameComponent>();
	flame->Initialize(coreRef->GetDevice(), positions.size());
	flame->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/T_candleflame.png");
	flame->SetParticleSize(0.35f);
	for (size_t i = 0; i < positions.size(); ++i)
		flame->Spawn(positions[i]);
	AddGameObject(flameObject);
#pragma endregion
}

void FinalBattleScene::InitializeSceneMonsters()
{
	const XMFLOAT3 monsterSpawn = { 22.f, SampleHeightAt(22.f, 22.f), 22.f };
	CreateMonsters(MonsterType::Boss, monsterSpawn, 1);

	CreateGimmickPool(MAX_CHARACTER_COUNT);	
}

void FinalBattleScene::UpdateScene(const float deltaTime)
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
		if (pos.x > -6.3f && pos.x < 6.01f && pos.z > -55.0f)
		{
			auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Final);
			if (controller)
				controller->ShowMapName();
		}
	}

	for (const auto& obj : gameObjects)
	{
		if (!obj->IsStatic())
			obj->Update(deltaTime);
	}

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

void FinalBattleScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}
}

void FinalBattleScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());
}

void FinalBattleScene::RenderSceneShadowStatic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowStatic(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadowStatic(*coreRef, renderer);
	}
}

void FinalBattleScene::RenderSceneShadowDynamic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowDynamic(*coreRef, gameObjects);
}

void FinalBattleScene::RenderSceneEffects()
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

void FinalBattleScene::RequestSceneChange()
{
}

const char* FinalBattleScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/CathedralBGM.mp3";
}

const char* FinalBattleScene::GetBossBGMPath() const
{
	return "../Assets/Music/BGM/FinalBossBGM.mp3";
}


float FinalBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	return 0.0f;
}
