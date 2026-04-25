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
#include "AnimationMachine.h"
#include "EffectComponent.h"
#include "AnimationSetFactory.h"
#include "Animator.h"
#include "NetId.h"
#include "NetHelper.h"
#include "EntityId.h"

void SecondBattleScene::Release()
{
}

void SecondBattleScene::Reset()
{
	instancingBatches.clear();
	monsterPools.clear();
	activeCharacters.clear();
	gameObjects.clear();
	myPlayer = nullptr;

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

	CreateKnightPool();

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
	coreRef->GetShadowMgr()->SetSkyBox(skyBox.get());
	coreRef->GetLightMgr()->UpdateLights();

#pragma region Initialize Castle Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"CastleMap/textures/CastleFloor", L"../Assets/FBXModel/CastleMap/castleTerrain.raw", 513, 650.2402f, 79.28662f, 8.0f);
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
	CreateMonsters(MonsterType::Imp, monsterSpawn, 5);
	CreateMonsters(MonsterType::DemonStriker, monsterSpawn, 5);
	CreateMonsters(MonsterType::DemonExecutioner, monsterSpawn, 5);
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

void SecondBattleScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
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

void SecondBattleScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void SecondBattleScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
{
	NetId nid{ add.netid() };
	int id = nid.GetId();
	int type = add.typeid_();

	if (type == static_cast<int>(CharacterId::FinalBoss)) // Final_Boss
	{
		auto boss = GetAvailableMonster(MonsterType::Boss);
		if (boss)
		{
			boss->SetId(id);
			auto transform = boss->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = boss;
		}
	}
	else if (type == static_cast<int>(CharacterId::Knight)) // Knight
	{
		auto player = GetAvailableKnight();
		if (player)
		{
			player->SetId(id);
			auto transform = player->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = player;

			if (id == INPUT.GetClientID())
			{
				myPlayer = player;
				myPlayer->SetAsLocalPlayer(cam.get());

				IMGUI.SetMyPlayer(myPlayer.get());

				OutputDebugStringA("My character activated!\n");
			}
		}
	}
	else if (type == static_cast<int>(CharacterId::Imp))
	{
		auto imp = GetAvailableMonster(MonsterType::Imp);
		if (imp)
		{
			imp->SetId(id);
			auto transform = imp->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = imp;
		}
	}
	else if (type == static_cast<int>(CharacterId::DemonStriker))
	{
		auto striker = GetAvailableMonster(MonsterType::DemonStriker);
		if (striker)
		{
			striker->SetId(id);
			auto transform = striker->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = striker;
		}
	}
	else if (type == static_cast<int>(CharacterId::DemonExecutioner))
	{
		auto demonExecutionerObject = GetAvailableMonster(MonsterType::DemonExecutioner);
		if (demonExecutionerObject)
		{
			demonExecutionerObject->SetId(id);
			auto transform = demonExecutionerObject->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = demonExecutionerObject;
		}
	}
}

void SecondBattleScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
{
	NetId nid{ move.netid() };
	int id = nid.GetId();
	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		auto transform = it->second->GetComponent<Transform>();

		transform->SetPosition(move.x(), move.y(), move.z());
		transform->SetTargetRotation(move.yaw());
	}
}

void SecondBattleScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void SecondBattleScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void SecondBattleScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
{
	const NetId nid{ stat.netid() };
	const int id = nid.GetId();

	const int curHp = stat.curhp();
	const int curStamina = stat.curstamina();

	const int maxHp = stat.maxhp();
	const int maxStamina = stat.maxstamina();

	const int power = stat.power();
	const int defense = stat.defense();
	const int mSpeed = stat.movespeed();
	const double aSpeed = stat.attackspeed();

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Castle);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}