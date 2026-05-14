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

void FinalBattleScene::Release()
{
	instancingBatches.clear();
	characterPools.clear();
	monsterPools.clear();
	activeCharacters.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	OutputDebugStringA("FinalBattleScene Data has been deleted!! \n----------------------------------------\n");
}

SceneSettings FinalBattleScene::GetSceneSettings() const
{
	return {
		  .light = { .sunDirection = { 0.0f, -0.75f, -1.0f }, .sunIntensity = 0.0f},
		  .lut = { .lutIndex = 1, .saturation = 1.0f },
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
	coreRef->GetLightMgr()->UpdateLights();

	const XMFLOAT3 candlePositions[] = { {2.824002f, 3.450002f, -57.236343f}, {7.456354f, 3.450002f, -45.545242f}, {6.765375f, 2.900002f, -39.137711f},
		{6.805631f, 2.300002f, -31.051842f}, {7.515741f, 3.450002f, -1.208803f}, {-7.392492f, 3.450002f, -0.762147f}, {-6.920892f, 2.300002f, -31.079567f},
		{-6.946253f, 2.900002f, -39.099716f}, {-7.372187f, 3.450002f, -45.562912f}, {-2.856723f, 3.450002f, -57.070786f} };
	auto flameObject = make_shared<GameObject>();
	flameObject->SetId(-1);
	auto flame = flameObject->AddComponent<FlameComponent>();
	flame->Initialize(coreRef->GetDevice(), 32);
	flame->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/T_candleflame.png");
	flame->SetParticleSize(1.0f);
	for (auto& pos : candlePositions)
		flame->Spawn(pos);
	AddGameObject(flameObject);
}

void FinalBattleScene::InitializeSceneMonsters()
{
	const XMFLOAT3 monsterSpawn = { 22.f, SampleHeightAt(22.f, 22.f), 22.f };
	CreateMonsters(MonsterType::Boss, monsterSpawn, 1);
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

float FinalBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	return 0.0f;
}
