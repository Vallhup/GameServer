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
#include "AnimationMachine.h"
#include "EffectComponent.h"
#include "NetId.h"
#include "NetHelper.h"
#include "EntityId.h"
#include "AnimationSetFactory.h"
#include "Animator.h"

void FirstBattleScene::Release()
{
}

void FirstBattleScene::Reset()
{
	instancingBatches.clear();
	activeCharacters.clear();
	gameObjects.clear();
	myPlayer = nullptr;
	bossObject = nullptr;

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

void FirstBattleScene::InitializeSceneObjectPools()
{
}

void FirstBattleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nFirstBattleScene Data has been created!! \n");

	// SceneManager에서 공유 캐릭터 받아오기
	myPlayer = sManagerRef->GetSharedKnight();
	//bossObject = sManagerRef->GetSharedBoss();

	if (myPlayer)
	{
		myPlayer->SetAsLocalPlayer(cam.get());
		activeCharacters[myPlayer->GetId()] = myPlayer;
		gameObjects.push_back(myPlayer);

		IMGUI.SetMyPlayer(myPlayer.get());
		OutputDebugStringA("FirstBattle: MyPlayer loaded from shared!\n");
	}

	if (bossObject)
	{
		activeCharacters[bossObject->GetId()] = bossObject;
		gameObjects.push_back(bossObject);
		OutputDebugStringA("FirstBattle: Boss loaded from shared!\n");
	}

	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"skybox1");
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());
	coreRef->GetLightMgr()->SetSkyBox(skyBox.get());
	coreRef->GetShadowMgr()->SetSkyBox(skyBox.get());
	coreRef->GetLightMgr()->UpdateLights();

#pragma region Initialize Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"../Assets/FBXModel/VillageMap/ground", L"../Assets/FBXModel/VillageMap/villageTerrain.raw", 513, 1023.0f, 159.4766f, 2.0f);
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

	CreateBossObject();

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	OutputDebugStringA("FirstBattleScene initialized!\n");
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

void FirstBattleScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
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

void FirstBattleScene::CreateBossObject()
{
	bossObject = make_shared<GameObject>();
	bossObject->SetId(-1);
	auto mesh = bossObject->AddComponent<Mesh>();
	auto transform = bossObject->AddComponent<Transform>();
	auto animator = bossObject->AddComponent<Animator>();
	auto animMachine = bossObject->AddComponent<AnimationMachine>();
	mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/DemonExecutioner/monster_DemonExecutioner");

	animMachine->SetAnimationSet(AnimationSetFactory::CreateDemonExecutionerSet());
	transform->SetInitPosition(22.f, SampleHeightAt(22.0f, 22.0f), 22.f);
	transform->SetRotation(0.f, 3.14f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);
	gameObjects.push_back(bossObject);
}

float FirstBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

void FirstBattleScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void FirstBattleScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
{
	NetId nid{ add.netid() };
	int id = nid.GetId();
	int type = add.typeid_();

	constexpr float offsetX = -352.0f;
	constexpr float offsetZ = 168.0f;

	float worldX = add.x() + offsetX;		// 임시 예측 좌표임 (맵 기반)
	float worldZ = add.z() + offsetZ;

	if (type == static_cast<int>(CharacterId::DemonExecutioner)) // Final_Boss
	{
		if (bossObject)
		{
			bossObject->SetId(id);
			auto transform = bossObject->GetComponent<Transform>();
			transform->SetInitPosition(worldX, SampleHeightAt(worldX, worldZ), worldZ);
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = bossObject;
		}
	}
	else if (type == static_cast<int>(CharacterId::Knight)) // Knight
	{
		auto player = myPlayer;
		if (player)
		{
			player->SetId(id);
			auto transform = player->GetComponent<Transform>();
			transform->SetInitPosition(worldX, SampleHeightAt(worldX, worldZ), worldZ);
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = player;
		}

		if (id == INPUT.GetClientID())
		{
			myPlayer = player;
			myPlayer->SetAsLocalPlayer(cam.get());

			IMGUI.SetMyPlayer(myPlayer.get());

			OutputDebugStringA("My character activated!\n");
		}
	}
}

void FirstBattleScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
{
	// 서버 -> First 맵 오프셋 (임시)
	constexpr float offsetX = -352.0f;
	constexpr float offsetZ = 168.0f;

	NetId nid{ move.netid() };
	int id = nid.GetId();
	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		auto transform = it->second->GetComponent<Transform>();

		float worldX = move.x() + offsetX;		// 임시 예측 좌표임 (맵 기반)
		float worldZ = move.z() + offsetZ;
		//transform->SetPosition(worldX, SampleHeightAt(worldX, worldZ), worldZ);
		transform->SetPosition(move.x(), move.y(), move.z());
		transform->SetTargetRotation(move.yaw());
	}
}

void FirstBattleScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void FirstBattleScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
{
	NetId nid{ anim.netid() };
	int id = nid.GetId();

	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		if (auto animMachine = it->second->GetComponent<AnimationMachine>())
		{
			uint32 serverAnimIdx = anim.curranim();
			uint32 startIdx = animMachine->GetAnimationSet()->GetStartIndex();
			string animName = animMachine->GetAnimationSet()->GetClipNameByIndex(serverAnimIdx - startIdx);

			animMachine->OnServerClipConfirm(animName);
		}
	}
}

void FirstBattleScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
{
	const NetId nid{ stat.netid() };
	const int id = nid.GetId();

	const int curHp = stat.curhp();
	const int curStamina = stat.curstamina();

	const int maxHp = stat.maxhp();
	const int maxStamina = stat.maxstamina();

	const int power = stat.power();
	const int defense = stat.defense();
	const double mSpeed = stat.movespeed();
	const double aSpeed = stat.attackspeed();

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Village);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}
