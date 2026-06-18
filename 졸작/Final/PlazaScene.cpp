#include "pch.h"
#include "PlazaScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "MainCharacter.h"
#include "Animator.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "SoundManager.h"
#include "ImGuiManager.h"
#include "SkyBox.h"
#include "LightManager.h"
#include "ShadowMappingManager.h"
#include "Terrain.h"
#include "EffectRenderer.h"
#include "EffectManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "EffectComponent.h"

void PlazaScene::Release()
{
	instancingBatches.clear();
	characterPools.clear();
	monsterPools.clear();
	activeCharacters.clear();
	activeMonsterTypes.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	SOUND_MANAGER->StopBGM(1.0f);

	OutputDebugStringA("PlazaScene Data has been deleted!! \n----------------------------------------\n");
}

SceneSettings PlazaScene::GetSceneSettings() const
{
	return {
		  .light = { .sunIntensity = 1.0f },
		  .lut = { .lutIndex = 105, .saturation = 1.3f },
		  .fog = { .density = 0.015f, .scattering = 0.8f, .maxSteps = 32, .maxDistance = 90.0f,
					  .jitterStrength = 1.0f, .groundHeight = 5.0f, .lightIntensity = 1.5f },
		  .skybox = { .tintColor = { 1.0f, 1.0f, 1.0f }, .saturation = 2.0f},
		  .shadow = { .shadowAmbientMin = 1.0f, .shadowFloor = 0.0f },
	};
}

void PlazaScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nPlazaScene Data has been created!! \n");

	cam->SetCursor(false);

	CreateCharacterPool(CharacterType::Knight);
	CreateCharacterPool(CharacterType::Lancer);
	CreateCharacterPool(CharacterType::Paladin);

	InitializeSceneEnvironments();

	OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	for (const auto& obj : gameObjects)
	{
		if (auto mesh = obj->GetComponent<Mesh>())
			mesh->ReleaseUploadBuffers();
	}

	OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");
}

void PlazaScene::InitializeSceneEnvironments()
{
	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"skybox");
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());
	coreRef->GetLightMgr()->SetSkyBox(skyBox.get());
	coreRef->GetShadowMgr()->SetLightMgr(coreRef->GetLightMgr());
	coreRef->GetLightMgr()->LoadSceneLights(L"../Assets/FBXModel/PlazaMap/PlazaLightData.txt", true);
	coreRef->GetLightMgr()->UpdateLights();

#pragma region Initialize Plaza Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"PlazaMap/textures/plazaFloor", L"../Assets/FBXModel/PlazaMap/plazaTerrain.raw", 512, 1016.0f, 27.01563f, 1.0f);
	terrain->LoadSplatmap(*coreRef, L"../Assets/FBXModel/PlazaMap/terrainAtlas.bin", L"../Assets/FBXModel/PlazaMap/textures/");
	cam->SetTerrain(terrain.get());
#pragma endregion
}

void PlazaScene::UpdateScene(const float deltaTime)
{
	if (myPlayer)	
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());

		if (INPUT.GetKeyDown('0'))
		{
			XMFLOAT3 pos = myPlayer->GetComponent<Transform>()->GetPosition();
			OutputDebugStringA(("MyPlayer Pos: " + to_string(pos.x) + ", " + to_string(pos.y) + ", " + to_string(pos.z) + "\n").c_str());
		}
	}

	if (myPlayer)
	{
		auto& pos = myPlayer->GetComponent<Transform>()->GetPosition();
		if (pos.x > 475.823f && pos.x < 478.102f && pos.z > 451.728f && pos.z < 534.614f)
		{
		    auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Plaza);
		    if (controller)
		        controller->ShowMapName();
		}
	}

	if (myPlayer)
	{
		constexpr float STATUE_CX = 443.97f;
		constexpr float STATUE_CZ = 489.29f;
		constexpr float INTERACT_RADIUS = 3.06f;  
		constexpr float UI_HEIGHT = 8.5f;          

		const XMFLOAT3& pos = myPlayer->GetComponent<Transform>()->GetPosition();
		const float dx = pos.x - STATUE_CX;
		const float dz = pos.z - STATUE_CZ;
		const float distSq = dx * dx + dz * dz;

		auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Plaza);
		if (controller)
		{
			if (distSq <= INTERACT_RADIUS * INTERACT_RADIUS)
			{
				const XMFLOAT3 anchor{ STATUE_CX, UI_HEIGHT, STATUE_CZ };
				controller->SetInteractPrompt(true, anchor);
			}
			else
			{
				controller->SetInteractPrompt(false, {});
			}
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

void PlazaScene::RenderSceneDeferred()
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

void PlazaScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());
}

void PlazaScene::RenderSceneShadowStatic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowStatic(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadowStatic(*coreRef, renderer);
	}
}

void PlazaScene::RenderSceneShadowDynamic()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadowDynamic(*coreRef, gameObjects);
}

void PlazaScene::RenderSceneEffects()
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

const char* PlazaScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/PlazaBGM.mp3";
}

float PlazaScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}