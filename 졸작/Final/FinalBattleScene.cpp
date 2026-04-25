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
#include "AnimationMachine.h"
#include "EffectManager.h"
#include "EffectComponent.h"
#include "FlameComponent.h"
#include "AnimationSetFactory.h"
#include "Animator.h"
#include "NetId.h"
#include "NetHelper.h"
#include "EntityId.h"

void FinalBattleScene::Release()
{
}

void FinalBattleScene::Reset()
{
	instancingBatches.clear();
	monsterPools.clear();
	activeCharacters.clear();
	gameObjects.clear();
	myPlayer = nullptr;

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

	CreateKnightPool();

	// ----------------------------------
	// 다찬이가 만든 보스 띄우는 임시 함수 - 중간발표용
	// ----------------------------------
	CreateBossCharacter();
	// ----------------------------------

	if (myPlayer)
	{
		myPlayer->SetAsLocalPlayer(cam.get());
		activeCharacters[myPlayer->GetId()] = myPlayer;
		AddGameObject(myPlayer);

		IMGUI.SetMyPlayer(myPlayer.get());
		OutputDebugStringA("FinalBattle: MyPlayer loaded from shared!\n");
	}

	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"skybox3");
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());
	coreRef->GetLightMgr()->SetSkyBox(skyBox.get());
	coreRef->GetShadowMgr()->SetSkyBox(skyBox.get());
	coreRef->GetLightMgr()->UpdateLights();

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

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

	OutputDebugStringA("FinalBattleScene initialized!\n");
}

void FinalBattleScene::InitializeSceneMonsters()
{
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

void FinalBattleScene::RenderSceneShadow()
{
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

void FinalBattleScene::CreateBossCharacter()
{
	auto first = make_shared<GameObject>();
	first->SetId(0);
	auto mesh = first->AddComponent<Mesh>();
	auto transform = first->AddComponent<Transform>();
	auto animator = first->AddComponent<Animator>();
	mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Boss/boss");

	transform->SetInitPosition(0.084400f, 2.06280f, 2.891023f);
	transform->SetRotation(0.f, 0.f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);
	AddGameObject(first);
}

float FinalBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	return 0.0f;
}

void FinalBattleScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void FinalBattleScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
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

void FinalBattleScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
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

void FinalBattleScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void FinalBattleScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void FinalBattleScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Final);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}