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
#include "AnimationSfxComponent.h"
#include "ParrySparkComponent.h"
#include "BloodImpactComponent.h"
#include "ParryFlashComponent.h"
#include "ParryStreakComponent.h"
#include "EffectManager.h"
#include "SwordSpecialEffectComponent.h"

#include "NetId.h"
#include "NetHelper.h"
#include "EntityId.h"
#include "SoundManager.h"

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
    if (!bgmStarted)
    {
        if (const char* bgm = GetBGMPath())
            SOUND_MANAGER->PlayBGM(bgm, GetBGMFadeInSeconds());
        bgmStarted = true;
    }

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
		case PacketType::SC_LOGIN_SUCCESS:
		{
			return NetHelper::DispatchPacket<Protocol::SC_LOGIN_SUCCESS_PACKET>(header, data,
				[this](const auto& packet) { HandleLoginSuccess(packet); });
		}
		case PacketType::SC_LOGIN_FAIL:
		{
			return NetHelper::DispatchPacket<Protocol::SC_LOGIN_FAIL_PACKET>(header, data,
				[this](const auto& packet) { HandleLoginFail(packet); });
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
		case PacketType::SC_COMBAT_IMPACT:
		{
			return NetHelper::DispatchPacket<Protocol::SC_COMBAT_IMPACT_PACKET>(header, data,
				[this](const auto& packet) { HandleCombatImpact(packet); });
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

shared_ptr<MainCharacter> Scene::GetAvailableCharacter(CharacterType type) const
{
	auto it = characterPools.find(type);
	if (it == characterPools.end()) return nullptr;
	for (auto& c : it->second)
		if (c->GetId() == -1) return c;
	return nullptr;
}

shared_ptr<GameObject> Scene::GetAvailableMonster(MonsterType type) const
{
	auto it = monsterPools.find(type);
	if (it == monsterPools.end()) return nullptr;
	for (auto& m : it->second)
		if (m->GetId() == -1) return m;
	return nullptr;
}

shared_ptr<MainCharacter> Scene::CreateCharacterObject(const wstring& meshPath, shared_ptr<AnimationSet>(*animFactory)())
{
	auto character = make_shared<MainCharacter>();
	character->SetId(-1);
	auto mesh = character->AddComponent<Mesh>();
	auto transform = character->AddComponent<Transform>();
	character->AddComponent<Animator>();
	auto animMachine = character->AddComponent<AnimationMachine>();

	mesh->SetMesh(*coreRef, meshPath);
	animMachine->SetAnimationSet(animFactory());
	transform->SetRotation(0.f, 0.f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);

	auto trail = character->AddComponent<TrailComponent>();
	trail->Initialize(coreRef->GetDevice(), 64);
	trail->SetColor({ 0.85f, 0.88f, 0.9f, 0.15f });
	trail->SetLifetime(0.3f);

	auto dust = character->AddComponent<FootDustComponent>();
	dust->Initialize(coreRef->GetDevice(), 32);
	dust->SetColor({ 0.15f, 0.15f, 0.15f, 1.0f });
	dust->SetLifetime(0.35f);
	dust->SetParticleSize(0.1f);

	auto flash = character->AddComponent<ParryFlashComponent>();
	flash->Initialize(coreRef->GetDevice(), 1);
	flash->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/ParrySpark2.png");
	flash->SetColor({ 6.0f, 1.8f, 0.15f, 0.5f });
	flash->SetSize(1.0f);
	flash->SetLifetime(0.10f);

	auto spark = character->AddComponent<ParrySparkComponent>();
	spark->Initialize(coreRef->GetDevice(), 128);
	spark->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/Flash01.png");
	spark->SetColor({ 4.0f, 0.05f, 0.02f, 3.0f });
	spark->SetSpeed(20.0f);
	spark->SetParticleSize(0.1f);
	spark->SetLifetime(0.75f);

	auto streak = character->AddComponent<ParryStreakComponent>();
	streak->Initialize(coreRef->GetDevice(), 1);
	streak->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/ParryStar.png");
	streak->SetColor({ 6.0f, 3.0f, 1.0f, 0.5f });
	streak->SetWidth(15.0f);
	streak->SetHeight(0.4f);
	streak->SetLifetime(0.1f);

	auto blood = character->AddComponent<BloodImpactComponent>();
	blood->InitializeBlood(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), 64);
	blood->SetColor({ 0.35f, 0.35f, 0.35f, 1.0f });
	blood->SetLifetime(1.2f);
	blood->SetBaseSpeed(5.5f);
	blood->SetBaseSize(1.2f);
	blood->SetGravity(0.0f);
	blood->SetDragHalfLife(0.5f);
	blood->SetCountPerSlot(5);

	// 검 본 추적 스페셜 이펙트 (쌍검 캐릭터는 본 2개 — CreateCharacterPool에서 설정)
	character->AddComponent<SwordSpecialEffectComponent>();

	return character;
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

	auto blood = obj->AddComponent<BloodImpactComponent>();
	blood->InitializeBlood(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), 64);
	blood->SetColor({ 0.35f, 0.35f, 0.35f, 1.0f });
	blood->SetLifetime(1.2f);
	blood->SetBaseSpeed(5.5f);
	blood->SetBaseSize(1.2f);
	blood->SetGravity(0.0f);
	blood->SetDragHalfLife(0.5f);
	blood->SetCountPerSlot(5);

	return obj;
}

void Scene::CreateCharacterPool(CharacterType type, int count)
{
	struct CharacterDesc
	{
		const wchar_t* meshPath;
		shared_ptr<AnimationSet>(*animFactory)();
	};

	static const unordered_map<CharacterType, CharacterDesc> descs = {
		{ CharacterType::Knight,  { L"../Assets/FBXModel/Knight/knight6",  &AnimationSetFactory::CreateKnightSet  } },
		{ CharacterType::Lancer,  { L"../Assets/FBXModel/Lancer/lancer",   &AnimationSetFactory::CreateLancerSet  } },
		{ CharacterType::Paladin, { L"../Assets/FBXModel/Paladin/paladin", &AnimationSetFactory::CreatePaladinSet } },
	};

	static const unordered_map<CharacterType, const wchar_t*> swordEffectNames = {
		{ CharacterType::Knight,  L"KnightSpecialAttack" },
		{ CharacterType::Lancer,  L"LancerSpecialAttack" },
		{ CharacterType::Paladin, L"PaladinSpecialAttack"  },
	};

	const auto& desc = descs.at(type);
	const wstring swordEffectName = swordEffectNames.at(type);
	EFFECT_MANAGER->PreLoad(swordEffectName);

	for (int i = 0; i < count; ++i)
	{
		auto character = CreateCharacterObject(desc.meshPath, desc.animFactory);
		character->GetComponent<Transform>()->SetInitPosition(-5.f + (1.f * (i % 10)), 0.f, 5.f);

		auto swordEffect = character->GetComponent<SwordSpecialEffectComponent>();
		swordEffect->SetEffectName(swordEffectName);

		auto trail = character->GetComponent<TrailComponent>();

		auto sfx = character->AddComponent<AnimationSfxComponent>();
		switch (type)
		{
		case CharacterType::Knight:
			sfx->AddTrigger("Walk", 5, 7, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Walk", 22, 24, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 6, 8, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 14, 16, "../Assets/Music/SFX/Foot.mp3");
			swordEffect->SetBoneIndices({ 45 });
			trail->SetBoneIndices({ 45 });
			trail->SetBladeLength(1.02f);
			break;
		case CharacterType::Lancer:
			swordEffect->SetBoneIndices({ 25, 45 });
			trail->SetBoneIndices({ 25, 45 });
			trail->SetBladeLength(0.81f);
			break;
		case CharacterType::Paladin:
			swordEffect->SetBoneIndices({ 44 });
			trail->SetBoneIndices({ 44 });
			trail->SetBladeLength(0.95f);
			break;
		}

		characterPools[type].push_back(character);
		AddGameObject(character);
	}
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

void Scene::HandleLoginSuccess(const Protocol::SC_LOGIN_SUCCESS_PACKET& success)
{
	NetId nid{ success.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	IMGUI.SetLoginSuccess(true);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void Scene::HandleLoginFail(const Protocol::SC_LOGIN_FAIL_PACKET& fail)
{
	// TODO : 실패 이유에 따라서 Log 띄워주기? 그냥 Dialog 처리?
	switch (fail.reason()) {
	default:
		OutputDebugStringA(("Login Fail, Reason : " + to_string(fail.reason())).c_str());
	}
}

void Scene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
{
	NetId nid{ add.netid() };
	int id = nid.GetId();
	int type = add.typeid_();

	static const unordered_map<int, MonsterType> monsterMap = {
		{ static_cast<int>(CharacterId::FinalBoss),        MonsterType::Boss             },
		{ static_cast<int>(CharacterId::Imp),              MonsterType::Imp              },
		{ static_cast<int>(CharacterId::DemonStriker),     MonsterType::DemonStriker     },
		{ static_cast<int>(CharacterId::DemonExecutioner), MonsterType::DemonExecutioner },
		{ static_cast<int>(CharacterId::BigDemonWarrior),  MonsterType::BigDemonWarrior  },
		{ static_cast<int>(CharacterId::Tank),  MonsterType::Tank  },
	};

	static const unordered_map<int, CharacterType> characterMap = {
		{ static_cast<int>(CharacterId::Knight),  CharacterType::Knight  },
		{ static_cast<int>(CharacterId::Lancer),  CharacterType::Lancer  },
		{ static_cast<int>(CharacterId::Paladin), CharacterType::Paladin },
	};

	if (auto monsterIter = monsterMap.find(type); monsterIter != monsterMap.end())
	{
		if (auto monster = GetAvailableMonster(monsterIter->second))
		{
			monster->SetId(id);
			auto transform = monster->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = monster;
			activeMonsterTypes[id] = monsterIter->second;
		}
	}
	else if (auto charcterIter = characterMap.find(type); charcterIter != characterMap.end())
	{
		if (auto player = GetAvailableCharacter(charcterIter->second))
		{
			player->SetId(id);
			auto transform = player->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = player;

			if (id == INPUT.GetClientID())
			{
				myPlayer = player;
				myCharacterType = charcterIter->second;
				myPlayer->SetAsLocalPlayer(cam.get());
				IMGUI.SetMyPlayer(myPlayer.get());
				if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
					controller->SetLocalCharacterType(myCharacterType);
				OutputDebugStringA("My character activated!\n");
			}
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
	const NetId nid{ remove.netid() };
	const int id = nid.GetId();

	auto it = activeCharacters.find(id);
	if (it == activeCharacters.end())
		return;

	it->second->SetId(-1);            
	activeCharacters.erase(it);
	activeMonsterTypes.erase(id);
}

void Scene::HandleCombatImpact(const Protocol::SC_COMBAT_IMPACT_PACKET& impact)
{
	const NetId aNetId{ impact.attackernetid() };
	const int aId = aNetId.GetId();

	const NetId vNetId{ impact.victimnetid() };
	const int vId = vNetId.GetId();

	const bool canImpact =
		activeCharacters.contains(aId) && activeCharacters.contains(vId);

	if (canImpact)
	{
		const XMFLOAT3 impactPos{ impact.impactx(), impact.impacty(), impact.impactz() };
		const XMFLOAT3 impactDir{ impact.dirx(), impact.diry(), impact.dirz() };

		// 0 : Hit / 1 : Guard / 2 : Parry
		switch (impact.resulttype()) {
		case 0:
		{
			auto victim = activeCharacters[vId];
			if (victim)
			{
				if (auto blood = victim->GetComponent<BloodImpactComponent>())
					blood->Spawn(impactPos, impactDir);

				if (auto typeIt = activeMonsterTypes.find(vId); typeIt != activeMonsterTypes.end())
				{
					if (typeIt->second == MonsterType::Boss)
						SOUND_MANAGER->PlaySFX3D("../Assets/Music/SFX/CutFinalBoss.mp3", impactPos);
					else
						SOUND_MANAGER->PlaySFX3D("../Assets/Music/SFX/CutMonster.mp3", impactPos);
				}
			}
			break;
		}
		case 1:
		{
			break;
		}
		case 2:
		{
			auto victim = activeCharacters[vId];
			if (victim)
			{
				if (auto flash = victim->GetComponent<ParryFlashComponent>())
					flash->Spawn(impactPos);
				if (auto spark = victim->GetComponent<ParrySparkComponent>())
					spark->Spawn(impactPos, 64);
				if (auto streak = victim->GetComponent<ParryStreakComponent>())
					streak->Spawn(impactPos);

				SOUND_MANAGER->PlaySFX3D("../Assets/Music/SFX/Parry.mp3", impactPos);
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}
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

			animMachine->OnServerClipConfirm(
				animName,
				anim.curranim(),
				anim.abilityinstanceid(),
				anim.normalizedtime());
		}
	}
}

void Scene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
{
	const NetId nid{ stat.netid() };
	const int id = nid.GetId();

	if (myPlayer && myPlayer->GetId() == id)
	{
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
	else if (auto typeIt = activeMonsterTypes.find(id);
		typeIt != activeMonsterTypes.end() &&
		(typeIt->second == MonsterType::Imp ||
		 typeIt->second == MonsterType::DemonStriker ||
		 typeIt->second == MonsterType::DemonExecutioner))
	{
		if (auto objIt = activeCharacters.find(id); objIt != activeCharacters.end())
			if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
				controller->HandleMonsterHp(id, objIt->second.get(), stat.curhp(), stat.maxhp());
	}
	else if (activeMonsterTypes.find(id) == activeMonsterTypes.end() &&
		activeCharacters.find(id) != activeCharacters.end())
	{
		if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
			controller->HandlePartyMemberHp(id, stat.curhp(), stat.maxhp());
	}
}
