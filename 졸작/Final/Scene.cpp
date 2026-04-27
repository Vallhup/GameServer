#include "pch.h"
#include "Scene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Engine.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "Material.h"
#include "Camera.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "Animator.h"
#include "AnimationMachine.h"
#include "AnimationSetFactory.h"
#include "TrailComponent.h"
#include "FootDustComponent.h"
#include "ParrySparkComponent.h"

#include "NetId.h"
#include "NetHelper.h"
#include "EntityId.h"

void Scene::Initialize(HWND hWnd, DX12Core& core)
{
    coreRef = &core;

    if (cam)
        cam.reset();

    cam = make_unique<Camera>();
    cam->Initialize(hWnd);

    InitializeLogic();

    coreRef->FlushCommandQueue();
    coreRef->ResetCommandQueue();

    Material::ReleaseUploadBuffers();
}

void Scene::Update(const float deltaTime)
{
    UpdateScene(deltaTime);
    
    // Temporarily test in GameScene Only
    //if (cam)
    //    cam->Update(*coreRef, deltaTime, );
    
    RequestSceneChange();
}

Camera* Scene::GetCamera() const
{
    return cam.get();
}

void Scene::SetSceneManager(SceneManager* manager)
{
    sManagerRef = manager;
}

void Scene::HandlePacket(const PacketHeader & header, const BYTE * data)
{
	PacketType type = static_cast<PacketType>(header.type);

	switch (type) {
		case PacketType::SC_LOGIN:
		{
			return NetHelper::DispatchPacket<Protocol::SC_LOGIN_PACKET>(header, data,
				[this](const auto& packet) { HandleLogin(packet); });
		}
		case PacketType::SC_ADD:
		{
			return NetHelper::DispatchPacket<Protocol::SC_ADD_PACKET>(header, data,
				[this](const auto& packet) { HandleAdd(packet); });
		}
		case PacketType::SC_MOVE_OBJECT:
		{
			return NetHelper::DispatchPacket<Protocol::SC_MOVE_PACKET>(header, data,
				[this](const auto& packet) { HandleMove(packet); });
		}
		case PacketType::SC_REMOVE:
		{
			return NetHelper::DispatchPacket<Protocol::SC_REMOVE_PACKET>(header, data,
				[this](const auto& packet) { HandleRemove(packet); });
		}
		case PacketType::SC_ANIMATION_CHANGE:
		{
			return NetHelper::DispatchPacket<Protocol::SC_ANIMATION_TRANSITION_PACKET>(header, data,
				[this](const auto& packet) { HandleAnimationChange(packet); });
		}
		case PacketType::SC_STAT_CHANGE:
		{
			return NetHelper::DispatchPacket<Protocol::SC_STAT_CHANGE_PACKET>(header, data,
				[this](const auto& packet) { HandleStatChange(packet); });
		}
	}
}

void Scene::SetInstancingBatches(vector<shared_ptr<InstancingBatch>>&& batches)
{
	instancingBatches = move(batches);
}

shared_ptr<GameObject> Scene::GetAvailableMonster(MonsterType type) const
{
	auto it = monsterPools.find(type);
	if (it == monsterPools.end()) return nullptr;
	for (auto& m : it->second)
		if (m->GetId() == -1) return m;
	return nullptr;
}

shared_ptr<MainCharacter> Scene::GetAvailableKnight() const
{
	for (auto& knight : knightPool)
	{
		if (knight->GetId() == -1)
			return knight;
	}
	return nullptr;
}

void Scene::CreateKnightPool()
{
	for (int i = 0; i < MAX_KNIGHT_COUNT; ++i)
	{
		auto knight = make_shared<MainCharacter>();
		knight->SetId(-1);
		auto mesh = knight->AddComponent<Mesh>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		auto animMachine = knight->AddComponent<AnimationMachine>();
		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");
		//mesh->SetCollisionMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");

		animMachine->SetAnimationSet(AnimationSetFactory::CreateKnightSet());
		transform->SetInitPosition(-5.f + (1.f * (i % 10)), 0.f, 5.f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);

		auto trail = knight->AddComponent<TrailComponent>();
		trail->Initialize(coreRef->GetDevice(), 32);
		trail->SetColor({ 1.0f, 0.6f, 0.2f, 1.0f });
		trail->SetLifetime(0.13f);

		auto dust = knight->AddComponent<FootDustComponent>();
		dust->Initialize(coreRef->GetDevice(), 32);
		dust->SetColor({ 0.15f, 0.15f, 0.15f, 0.4f });
		dust->SetLifetime(0.35f);
		dust->SetParticleSize(0.1f);

		auto spark = knight->AddComponent<ParrySparkComponent>();
		spark->Initialize(coreRef->GetDevice(), 64);
		spark->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/Flash01.png");
		spark->SetColor({ 4.0f, 0.05f, 0.02f, 3.0f });
		spark->SetSpeed(20.0f);
		spark->SetParticleSize(0.1f);
		spark->SetLifetime(0.75f);

		knightPool.push_back(knight);
		AddGameObject(knight);
	}
}

shared_ptr<GameObject> Scene::CreateMonsterObject(const wstring& meshPath, shared_ptr<AnimationSet> (*animFactory)(), bool twoSided)
{
	auto obj = make_shared<GameObject>();
	obj->SetId(-1);
	auto mesh = obj->AddComponent<Mesh>();
	auto transform = obj->AddComponent<Transform>();
	obj->AddComponent<Animator>();
	auto animMachine = obj->AddComponent<AnimationMachine>();

	mesh->SetMesh(*coreRef, meshPath);
	mesh->SetTwoSided(twoSided);
	animMachine->SetAnimationSet(animFactory());
	transform->SetRotation(0.f, 0.f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);
	return obj;
}

void Scene::CreateMonsters(MonsterType type, const XMFLOAT3& position, int count)
{
	struct MonsterDesc
	{
		const wchar_t* meshPath;
		shared_ptr<AnimationSet>(*animFactory)();
		bool twoSided;
	};

	static const unordered_map<MonsterType, MonsterDesc> descs = {
		{ MonsterType::Boss, { L"../Assets/FBXModel/Boss/boss", &AnimationSetFactory::CreateFinalBossSet, false } },
		{ MonsterType::Imp, { L"../Assets/FBXModel/Monster/Imp/monster_Imp", &AnimationSetFactory::CreateImpSet, true  } },
		{ MonsterType::DemonStriker, { L"../Assets/FBXModel/Monster/DemonStriker/monster_DemonStriker", &AnimationSetFactory::CreateDemonStrikerSet, true  } },
		{ MonsterType::DemonExecutioner, { L"../Assets/FBXModel/Monster/DemonExecutioner/monster_DemonExecutioner", &AnimationSetFactory::CreateDemonExecutionerSet, true  } },
		{ MonsterType::BigDemonWarrior, { L"../Assets/FBXModel/Monster/BigDemonWarrior/monster_BigDemonWarrior",&AnimationSetFactory::CreateBigDemonWarriorSet, true  } },
		{ MonsterType::Tank, { L"../Assets/FBXModel/Monster/Tank/monster_Tank", &AnimationSetFactory::CreateTankSet, true  } },
	};

	const auto& desc = descs.at(type);
	for (int i = 0; i < count; ++i)
	{
		auto monster = CreateMonsterObject(desc.meshPath, desc.animFactory, desc.twoSided);
		monster->GetComponent<Transform>()->SetInitPosition(position);
		monsterPools[type].push_back(monster);
		AddGameObject(monster);
	}
}

void Scene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void Scene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void Scene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
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
	else if (type == static_cast<int>(CharacterId::BigDemonWarrior))
	{
		auto bigDemonWarriorObject = GetAvailableMonster(MonsterType::BigDemonWarrior);
		if (bigDemonWarriorObject)
		{
			bigDemonWarriorObject->SetId(id);
			auto transform = bigDemonWarriorObject->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = bigDemonWarriorObject;
		}
	}
}

void Scene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
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

void Scene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void Scene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void Scene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>();
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}
