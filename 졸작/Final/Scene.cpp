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
#include "PotionAttachComponent.h"
#include "RunDustEffectComponent.h"
#include "DissolveComponent.h"
#include "GimmickDiamond.h"

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
	if (UI_MANAGER->IsVideoPlaying())
		return;

	if (!bgmStarted)
	{
		if (const char* bgm = GetBGMPath())
			SOUND_MANAGER->PlayBGM(bgm, GetBGMFadeInSeconds());
		bgmStarted = true;
	}

	UpdateScene(deltaTime);

	for (auto& [id, diamond] : activeGimmicks)
		diamond->Update(deltaTime);

	UpdateBreakerShields();

	UpdateTitleEffects();

	UpdateBuffEffects();

	UpdateDissolves();

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

void Scene::HandlePacket(const PacketHeader& header, const BYTE* data)
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
		case PacketType::SC_ITEM_COUNT:
		{
			return NetHelper::DispatchPacket<Protocol::SC_ITEM_COUNT_PACKET>(header, data,
				[this](const auto& packet) { HandleItemCount(packet); });
		}
		case PacketType::SC_TEAM_DEATH_COUNT:
		{
			return NetHelper::DispatchPacket<Protocol::SC_TEAM_DEATH_COUNT_PACKET>(header, data,
				[this](const auto& packet) { HandleTeamDeathCount(packet); });
		}
		case PacketType::SC_MONSTER_COMBAT_STATE:
		{
			return NetHelper::DispatchPacket<Protocol::SC_MONSTER_COMBAT_STATE_PACKET>(header, data,
				[this](const auto& packet) { HandleMonsterCombatState(packet); });
		}
		case PacketType::SC_BOSS_GIMMICK_OBJECT_SYNC:
		{
			return NetHelper::DispatchPacket<Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET>(header, data,
				[this](const auto& packet) { HandleBossGimmickObjectSync(packet); });
		}
		case PacketType::SC_BOSS_GIMMICK_ZONE_SYNC:
		{
			return NetHelper::DispatchPacket<Protocol::SC_BOSS_GIMMICK_ZONE_SYNC_PACKET>(header, data,
				[this](const auto& packet) { HandleBossGimmickZoneSync(packet); });
		}
		case PacketType::SC_FINAL_CLEAR_CHOICE_BEGIN:
		{
			return NetHelper::DispatchPacket<Protocol::SC_FINAL_CLEAR_CHOICE_BEGIN_PACKET>(header, data,
				[this](const auto& packet) { HandleFinalClearChoiceBegin(packet); });
		}
		case PacketType::SC_FINAL_CLEAR_CHOICE_RESULT:
		{
			return NetHelper::DispatchPacket<Protocol::SC_FINAL_CLEAR_CHOICE_RESULT_PACKET>(header, data,
				[this](const auto& packet) { HandleFinalClearChoiceResult(packet); });
		}
		case PacketType::SC_PVP_ROUND_RESULT:
		{
			return NetHelper::DispatchPacket<Protocol::SC_PVP_ROUND_RESULT_PACKET>(header, data,
				[this](const auto& packet) { HandlePvpRoundResult(packet); });
		}
		case PacketType::SC_GAMEPLAY_EFFECT_SYNC:
		{
			return NetHelper::DispatchPacket<Protocol::SC_GAMEPLAY_EFFECT_SYNC_PACKET>(header, data,
				[this](const auto& packet) { HandleGameplayEffectSync(packet); });
		}
		case PacketType::SC_TITLE_REPLICATION:
		{
			return NetHelper::DispatchPacket<Protocol::SC_TITLE_REPLICATION_PACKET>(header, data,
				[this](const auto& packet) { HandleTitleReplication(packet); });
		}
		case PacketType::SC_BEACON_CINEMATIC_START:
		{
			return NetHelper::DispatchPacket<Protocol::SC_BEACON_CINEMATIC_START_PACKET>(header, data,
				[this](const auto& packet) { HandleBeaconCinematicStart(packet); });
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

	// FootDust 비활성화 — RunDustEffectComponent(Effekseer)로 대체
	//auto dust = character->AddComponent<FootDustComponent>();
	//dust->Initialize(coreRef->GetDevice(), 32);
	//dust->SetColor({ 0.15f, 0.15f, 0.15f, 1.0f });
	//dust->SetLifetime(0.35f);
	//dust->SetParticleSize(0.1f);

	auto flash = character->AddComponent<ParryFlashComponent>();
	flash->Initialize(coreRef->GetDevice(), 1);
	flash->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/ParrySpark2.png");
	flash->SetColor({ 6.0f, 1.8f, 0.15f, 0.5f });
	flash->SetSize(1.0f);
	flash->SetLifetime(0.1f);

	auto spark = character->AddComponent<ParrySparkComponent>();
	spark->Initialize(coreRef->GetDevice(), 128);
	spark->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/Flash01.png");
	spark->SetColor({ 4.0f, 0.05f, 0.02f, 3.0f });
	spark->SetSpeed(20.0f);
	spark->SetParticleSize(0.1f);
	spark->SetLifetime(1.5f);

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

	character->AddComponent<SwordSpecialEffectComponent>();

	character->AddComponent<RunDustEffectComponent>();

	character->AddComponent<DissolveComponent>();
	DissolveComponent::RegisterNoiseTexture(*coreRef);

	auto potion = make_shared<GameObject>();
	potion->SetId(-1);
	auto potionMesh = potion->AddComponent<Mesh>();
	potion->AddComponent<Transform>();
	potionMesh->SetMesh(*coreRef, L"../Assets/FBXModel/Potion/potion");
	AddGameObject(potion);

	auto potionAttach = character->AddComponent<PotionAttachComponent>();
	potionAttach->SetPotion(potion);

	return character;
}

shared_ptr<GameObject> Scene::CreateMonsterObject(const wstring& meshPath, shared_ptr<AnimationSet>(*animFactory)(), bool twoSided)
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

	obj->AddComponent<DissolveComponent>();
	DissolveComponent::RegisterNoiseTexture(*coreRef);

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
	EFFECT_MANAGER->PreLoad(L"RunDust");

	EFFECT_MANAGER->PreLoad(L"Title1");
	EFFECT_MANAGER->PreLoad(L"Title2");
	EFFECT_MANAGER->PreLoad(L"Title3");
	EFFECT_MANAGER->PreLoad(L"Title4");
	EFFECT_MANAGER->PreLoad(L"Title5");
	EFFECT_MANAGER->PreLoad(L"Title6");
	EFFECT_MANAGER->PreLoad(L"Title7");
	EFFECT_MANAGER->PreLoad(L"Title8");

	EFFECT_MANAGER->PreLoad(L"Healing");
	EFFECT_MANAGER->PreLoad(L"Attack Increase");
	EFFECT_MANAGER->PreLoad(L"Max HP Increase");
	EFFECT_MANAGER->PreLoad(L"Defense Increase");
	EFFECT_MANAGER->PreLoad(L"Potion");

	for (int i = 0; i < count; ++i)
	{
		auto character = CreateCharacterObject(desc.meshPath, desc.animFactory);
		character->GetComponent<Transform>()->SetInitPosition(-5.f + (1.f * (i % 10)), 0.f, 5.f);

		auto swordEffect = character->GetComponent<SwordSpecialEffectComponent>();
		swordEffect->SetEffectName(swordEffectName);

		auto runDust = character->GetComponent<RunDustEffectComponent>();

		auto trail = character->GetComponent<TrailComponent>();

		auto potionAttach = character->GetComponent<PotionAttachComponent>();

		auto sfx = character->AddComponent<AnimationSfxComponent>();

		switch (type)
		{
		case CharacterType::Knight:
			sfx->AddTrigger("Walk", 5, 7, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Walk", 22, 24, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 5, 7, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 13, 15, "../Assets/Music/SFX/Foot.mp3");
			runDust->AddFootstep(13, 15, 50);    
			runDust->AddFootstep(5, 7, 54);  
			sfx->AddTrigger("AttackCombo1", 8, 10, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo2", 11, 13, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo3", 6, 8, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackSpecial", 29, 31, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("Dodge", 9, 11, "../Assets/Music/SFX/Roll.mp3");
			swordEffect->SetBoneIndices({ 45 });
			trail->SetBoneIndices({ 45 });
			trail->SetBladeLength(1.02f);
			potionAttach->SetBoneIndex(10);
			sfx->AddEffectTrigger("Drink", 40, 42, L"Potion");
			break;
		case CharacterType::Lancer:
			sfx->AddTrigger("Walk", 9, 11, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Walk", 25, 27, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 0, 2, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 8, 10, "../Assets/Music/SFX/Foot.mp3");
			runDust->AddFootstep(8, 10, 50);
			runDust->AddFootstep(0, 2, 54);
			sfx->AddTrigger("AttackCombo1", 16, 18, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo1", 25, 27, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo2", 14, 16, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo3", 18, 200, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackSpecial", 27, 29, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("Dodge", 8, 10, "../Assets/Music/SFX/Roll.mp3");
			swordEffect->SetBoneIndices({ 25, 45 });
			trail->SetBoneIndices({ 25, 45 });
			trail->SetBladeLength(0.81f);
			potionAttach->SetBoneIndex(9);
			potionAttach->SetAttachOffset(10.0f, 2.0f, 2.8f);
			sfx->AddEffectTrigger("Drink", 35, 37, L"Potion");
			break;
		case CharacterType::Paladin:
			sfx->AddTrigger("Walk", 10, 12, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Walk", 25, 27, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 0, 2, "../Assets/Music/SFX/Foot.mp3");
			sfx->AddTrigger("Run", 8, 10, "../Assets/Music/SFX/Foot.mp3");
			runDust->AddFootstep(8, 10, 49);
			runDust->AddFootstep(0, 2, 53);
			sfx->AddTrigger("AttackCombo1", 20, 22, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo2", 15, 17, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackCombo3", 19, 21, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("AttackSpecial", 23, 25, "../Assets/Music/SFX/SwingSword.mp3");
			sfx->AddTrigger("Dodge", 9, 11, "../Assets/Music/SFX/Roll.mp3");
			swordEffect->SetBoneIndices({ 44 });
			trail->SetBoneIndices({ 44 });
			trail->SetBladeLength(0.95f);
			potionAttach->SetBoneIndex(9);
			sfx->AddEffectTrigger("Drink", 38, 40, L"Potion");
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

		if (type == MonsterType::Boss || type == MonsterType::BigDemonWarrior || type == MonsterType::Tank)
			monster->GetComponent<DissolveComponent>()->UseBossNoise(true);

		if (type == MonsterType::Boss)
		{
			EFFECT_MANAGER->PreLoad(L"BloodLance");
			EFFECT_MANAGER->PreLoad(L"HolySandstorm");
			EFFECT_MANAGER->PreLoad(L"Sword_Moonlight");
			EFFECT_MANAGER->PreLoad(L"Sword_Storm");
			EFFECT_MANAGER->PreLoad(L"Fire");
			EFFECT_MANAGER->PreLoad(L"PhantasmMeteor_Single");
			EFFECT_MANAGER->PreLoad(L"Barrior02_HDR");
			EFFECT_MANAGER->PreLoad(L"Barrior01");
			EFFECT_MANAGER->PreLoad(L"Barrior03");
			EFFECT_MANAGER->PreLoad(L"0per_Bomb");

			auto sfx = monster->AddComponent<AnimationSfxComponent>();
			sfx->AddEffectTrigger("BloodLance", 79, 81, L"BloodLance");
			sfx->AddEffectTrigger("HolySandstorm", 0, 2, L"HolySandstorm");
			sfx->AddEffectTrigger("SwordMoonlight", 0, 2, L"Sword_Moonlight");
			sfx->AddEffectTrigger("SwordStorm", 0, 2, L"Sword_Storm");
			sfx->AddEffectTrigger("50per", 46, 48, L"Fire");
			sfx->AddEffectTrigger("0per", 28, 30, L"PhantasmMeteor_Single");
			sfx->AddEffectTrigger("0per", 0, 2, L"Barrior02_HDR");
			sfx->AddEffectTrigger("0per", 330, 332, L"0per_Bomb");

			sfx->AddTrigger("Thrust", 18, 20, "../Assets/Music/SFX/FinalBossSword.mp3");
			sfx->AddTrigger("Slash", 24, 26, "../Assets/Music/SFX/FinalBossSword.mp3");
			sfx->AddTrigger("DashSlash", 16, 18, "../Assets/Music/SFX/FinalBossSword.mp3");
			sfx->AddTrigger("JumpSlash", 27, 29, "../Assets/Music/SFX/FinalBossSword.mp3");
			sfx->AddTrigger("SwordMoonlight", 10, 12, "../Assets/Music/SFX/FinalBossSword.mp3");
			sfx->AddTrigger("SwordMoonlight", 30, 32, "../Assets/Music/SFX/FinalBossSword.mp3");
			sfx->AddTrigger("SwordMoonlight", 55, 57, "../Assets/Music/SFX/FinalBossSword.mp3");
			//sfx->AddTrigger("SwordStorm", 65, 67, "../Assets/Music/SFX/FinalBossSword.mp3");

			sfx->AddTrigger("BloodLance", 129, 131, "../Assets/Music/SFX/BloodlanceSound.mp3");
			sfx->AddTrigger("HolySandstorm", 45, 47, "../Assets/Music/SFX/HolySandstormSound.mp3");
			sfx->AddTrigger("SwordMoonlight", 21, 23, "../Assets/Music/SFX/SwordMoonlightSound.mp3");
			sfx->AddTrigger("SwordMoonlight", 42, 44, "../Assets/Music/SFX/SwordMoonlightSound.mp3");
			sfx->AddTrigger("SwordMoonlight", 67, 69, "../Assets/Music/SFX/SwordMoonlightSound.mp3");
			sfx->AddTrigger("SwordStorm", 70, 72, "../Assets/Music/SFX/SwordStormSound.mp3");
			sfx->AddTrigger("50per", 70, 72, "../Assets/Music/SFX/50perGathering.mp3");
			sfx->AddTrigger("50per", 346, 348, "../Assets/Music/SFX/50perExplosion.mp3");
			sfx->AddTrigger("0per", 28, 30, "../Assets/Music/SFX/0perGathering.mp3");
			sfx->AddTrigger("0per", 329, 331, "../Assets/Music/SFX/0perExplosion.mp3");

			auto trail = monster->AddComponent<TrailComponent>();
			trail->Initialize(coreRef->GetDevice(), 64);
			trail->SetColor({ 0.85f, 0.88f, 0.9f, 0.15f });
			trail->SetLifetime(0.3f);
			trail->SetBoneIndices({ 45 });  
			trail->SetBladeLength(1.92f);    
			trail->SetActivationClips({ "Slash", "DashSlash", "JumpSlash", "MultiSlash", "SwordMoonlight", "SwordStorm" });
		}

		if (type == MonsterType::Tank)
		{
			EFFECT_MANAGER->PreLoad(L"Tank_Jump");
			EFFECT_MANAGER->PreLoad(L"Tank_Attack");

			auto sfx = monster->AddComponent<AnimationSfxComponent>();
			sfx->AddEffectTrigger("Jump2", 70, 72, L"Tank_Jump");
			sfx->AddEffectTrigger("Melee4", 25, 27, L"Tank_Attack");
			sfx->AddEffectTrigger("Melee5", 33, 35, L"Tank_Attack");
			sfx->AddTrigger("Melee4", 24, 26, "../Assets/Music/SFX/TankBang.mp3");
			sfx->AddTrigger("Melee5", 32, 34, "../Assets/Music/SFX/TankBang.mp3");
			sfx->AddTrigger("Jump2", 70, 72, "../Assets/Music/SFX/TankJump.mp3");
		}

		if (type == MonsterType::BigDemonWarrior)
		{
			auto sfx = monster->AddComponent<AnimationSfxComponent>();
			sfx->AddTrigger("BattleCry", 1, 3, "../Assets/Music/SFX/VillageBossRoar.mp3");
			sfx->AddTrigger("Melee3", 28, 30, "../Assets/Music/SFX/VillageBossSwing.mp3");
			sfx->AddTrigger("Melee3", 48, 50, "../Assets/Music/SFX/VillageBossSwing.mp3");
			sfx->AddTrigger("Melee4", 29, 31, "../Assets/Music/SFX/VillageBossSwing.mp3");
			sfx->AddTrigger("Melee7", 33, 35, "../Assets/Music/SFX/VillageBossSwing.mp3");
			sfx->AddTrigger("Melee8", 20, 22, "../Assets/Music/SFX/VillageBossSwing.mp3");
		}

		monsterPools[type].push_back(monster);
		AddGameObject(monster);
	}
}

void Scene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void Scene::CreateGimmickPool(int count)
{
	for (int i = 0; i < count; ++i)
	{
		auto diamond = make_shared<GimmickDiamond>();
		diamond->SetId(-1);
		diamond->Init(*coreRef, XMFLOAT4{ 0.25f, 0.85f, 0.95f, 1.0f });
		diamond->AddComponent<DissolveComponent>();
		gimmickPool.push_back(diamond);
		AddGameObject(diamond);
	}
	DissolveComponent::RegisterNoiseTexture(*coreRef);
}

shared_ptr<GimmickDiamond> Scene::GetAvailableGimmick()
{
	for (auto& diamond : gimmickPool)
		if (diamond->GetId() == -1)
			return diamond;
	return nullptr;
}

void Scene::HandleLoginSuccess(const Protocol::SC_LOGIN_SUCCESS_PACKET& success)
{
	NetId nid{ success.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	IMGUI.OnLoginSucceeded();
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void Scene::HandleLoginFail(const Protocol::SC_LOGIN_FAIL_PACKET& fail)
{
	OutputDebugStringA(("Login Fail, Reason : " + to_string(fail.reason()) + "\n").c_str());

	IMGUI.OnLoginFailed(fail.reason());
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
			if (auto* dis = monster->GetComponent<DissolveComponent>()) dis->Reset();
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
			if (auto* dis = player->GetComponent<DissolveComponent>()) dis->Reset();
			auto transform = player->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetRotation(0.f, add.yaw(), 0.f);
			activeCharacters[id] = player;

			if (id == INPUT.GetClientID())
			{
				myPlayer = player;
				myCharacterType = charcterIter->second;
				myPlayer->SetAsLocalPlayer(cam.get(), add.yaw());
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

	if (auto typeIt = activeMonsterTypes.find(id); typeIt != activeMonsterTypes.end())
	{
		const MonsterType mt = typeIt->second;
		const bool isBoss = (mt == MonsterType::Boss || mt == MonsterType::BigDemonWarrior || mt == MonsterType::Tank);

		if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
		{
			if (isBoss)
				controller->RemoveBossHpBar();
			else
				controller->RemoveMonsterBar(id);
		}

		if (isBoss)
			OnBossDefeated();	

		if (auto* dis = it->second->GetComponent<DissolveComponent>())
			dis->Start();
	}
}

void Scene::UpdateBreakerShields()
{
	if (activeBreakerShields.empty()) return;

	bool gimmickActive = false;
	if (auto it = activeCharacters.find(shieldGateBossId); it != activeCharacters.end())
		if (auto* anim = it->second->GetComponent<AnimationMachine>())
			gimmickActive = anim->IsPlaying("50per");

	for (auto it = activeBreakerShields.begin(); it != activeBreakerShields.end(); )
	{
		const int breakerId = it->first;
		const int handle = it->second;

		if (!gimmickActive || !EFFECT_MANAGER->Exists(handle))
		{
			EFFECT_MANAGER->Stop(handle);
			it = activeBreakerShields.erase(it);
			continue;
		}

		if (auto cit = activeCharacters.find(breakerId); cit != activeCharacters.end())
			EFFECT_MANAGER->SetLocation(handle, cit->second->GetComponent<Transform>()->GetPosition());

		++it;
	}
}

void Scene::ReturnActiveObjectsToPool()
{
	for (auto& [id, obj] : activeCharacters)
		obj->SetId(-1);
	activeCharacters.clear();
	activeMonsterTypes.clear();

	for (auto& [id, gimmick] : activeGimmicks)
		gimmick->SetId(-1);
	activeGimmicks.clear();

	myPlayer = nullptr;
}

void Scene::UpdateDissolves()
{
	for (auto it = activeCharacters.begin(); it != activeCharacters.end(); )
	{
		auto* dis = it->second->GetComponent<DissolveComponent>();
		if (dis && dis->IsFinished())
		{
			it->second->SetId(-1);
			activeMonsterTypes.erase(it->first);
			it = activeCharacters.erase(it);
		}
		else
			++it;
	}

	for (auto it = activeGimmicks.begin(); it != activeGimmicks.end(); )
	{
		if (it->second->ShouldRemove())
		{
			it->second->SetId(-1);
			it = activeGimmicks.erase(it);
		}
		else
			++it;
	}
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

		const bool isMine = myPlayer && myPlayer->GetId() == vId;

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
				else
				{
					SOUND_MANAGER->PlaySFX3D("../Assets/Music/SFX/CharacterCut.mp3", impactPos);
				}

			}
			break;
		}
		case 1:
		{
			SOUND_MANAGER->PlaySFX3D("../Assets/Music/SFX/Guard.mp3", impactPos);
			break;
		}
		case 2:
		{
			if (isMine && cam)
				cam->AddTrauma(0.65f);   

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

			if (myPlayer && myPlayer->GetId() == id && cam && animName == "Dodge")
				cam->TriggerDodgeZoom();
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
	else if (auto bossIt = activeMonsterTypes.find(id);
		bossIt != activeMonsterTypes.end() &&
		(bossIt->second == MonsterType::Boss ||
			bossIt->second == MonsterType::BigDemonWarrior ||
			bossIt->second == MonsterType::Tank))
	{
		if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
			controller->HandleBossHp(stat.curhp(), stat.maxhp());
	}
	else if (activeMonsterTypes.find(id) == activeMonsterTypes.end() &&
		activeCharacters.find(id) != activeCharacters.end())
	{
		if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
			controller->HandlePartyMemberHp(id, stat.curhp(), stat.maxhp());
	}
}

void Scene::HandleItemCount(const Protocol::SC_ITEM_COUNT_PACKET& itemCount)
{
	uint32_t hpPotionCount = itemCount.hppotioncount();

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>();
	if (!controller) return;

	controller->SetPotionCount(hpPotionCount);
}

void Scene::HandleTeamDeathCount(const Protocol::SC_TEAM_DEATH_COUNT_PACKET& deathCount)
{
	uint32_t deathCnt = deathCount.deathcount();
	uint32_t maxDeathCount = deathCount.maxdeathcount();

	auto* ui = ENGINE.GetUIManager();
	for (SceneType st : { SceneType::Village, SceneType::Castle, SceneType::Final })
	{
		if (auto* c = ui->GetController<GameSceneUIController>(st))
			c->SetDeathCount(deathCnt, maxDeathCount);
	}
}

void Scene::HandleMonsterCombatState(const Protocol::SC_MONSTER_COMBAT_STATE_PACKET& combatState)
{
	NetId netId{ combatState.netid() };
	int id = netId.GetId();
	bool inCombat = combatState.incombat();

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>();
	if (!controller) return;

	auto typeIt = activeMonsterTypes.find(id);
	if (typeIt != activeMonsterTypes.end() &&
		(typeIt->second == MonsterType::Boss ||
			typeIt->second == MonsterType::BigDemonWarrior ||
			typeIt->second == MonsterType::Tank))
	{
		controller->SetBossCombatState(inCombat, typeIt->second);

		if (const char* bossBgm = GetBossBGMPath())
			SOUND_MANAGER->PlayBGM(inCombat ? bossBgm : GetBGMPath(), 0.5f);
	}
	else
		controller->SetMonsterCombatState(id, inCombat);
}

void Scene::HandleBossGimmickObjectSync(const Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET& gimmickObject)
{
	{
		const char* stateStr = "?";
		switch (gimmickObject.state())
		{
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED:   stateStr = "SPAWNED";   break;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_UPDATED:   stateStr = "UPDATED";   break;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN:    stateStr = "BROKEN";    break;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED: stateStr = "DESPAWNED"; break;
		}
		char buf[256];
		sprintf_s(buf,
			"[GimmickObjSync] boss=%d obj=%d seq=%u state=%s pos=(%.1f, %.1f, %.1f) r=%.2f hp=%u/%u\n",
			NetId{ gimmickObject.bossnetid() }.GetId(),
			NetId{ gimmickObject.objectnetid() }.GetId(),
			gimmickObject.gimmickseq(), stateStr,
			gimmickObject.x(), gimmickObject.y(), gimmickObject.z(),
			gimmickObject.radius(), gimmickObject.curhp(), gimmickObject.maxhp());
		OutputDebugStringA(buf);
	}

	const int objectId = NetId{ gimmickObject.objectnetid() }.GetId();

	if (gimmickObject.state() == Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN)
	{
		shieldGateBossId = NetId{ gimmickObject.bossnetid() }.GetId();
		const int breakerId = NetId{ gimmickObject.brokenbyplayernetid() }.GetId();
		if (auto bit = activeCharacters.find(breakerId); bit != activeCharacters.end())
			if (activeBreakerShields.find(breakerId) == activeBreakerShields.end())
			{
				const XMFLOAT3& pos = bit->second->GetComponent<Transform>()->GetPosition();
				activeBreakerShields[breakerId] = EFFECT_MANAGER->Play(L"Barrior03", pos);
			}
	}

	if (auto it = activeGimmicks.find(objectId); it != activeGimmicks.end())
	{
		it->second->SyncFrom(gimmickObject);
		return;
	}

	if (gimmickObject.state() != Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED) return;

	auto diamond = GetAvailableGimmick();
	if (!diamond) return;

	diamond->SetId(objectId);
	diamond->SyncFrom(gimmickObject);
	activeGimmicks[objectId] = diamond;
}

void Scene::HandleBossGimmickZoneSync(const Protocol::SC_BOSS_GIMMICK_ZONE_SYNC_PACKET& gimmickZone)
{
	// TODO: 0% 생존 영역 동기화
	//       별도의 Add, Remove Packet 없이 해당 패킷으로 모두 동기화 함
	const NetId bossNetId{ gimmickZone.bossnetid() };
	const int bossId = bossNetId.GetId();

	const uint32_t gimmickSeq = gimmickZone.gimmickseq();

	const NetId zoneNetId{ gimmickZone.zonenetid() };
	const int zoneId = zoneNetId.GetId();

	const Protocol::BossGimmickObjectState objectState = gimmickZone.state();

	const XMFLOAT3 objectPos{ gimmickZone.x(), gimmickZone.y(), gimmickZone.z() };
	const float objectRadius = gimmickZone.radius();

	{
		const char* stateStr = "?";
		switch (objectState)
		{
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED:   stateStr = "SPAWNED";   break;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_UPDATED:   stateStr = "UPDATED";   break;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN:    stateStr = "BROKEN";    break;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED: stateStr = "DESPAWNED"; break;
		}
		char buf[256];
		sprintf_s(buf,
			"[GimmickZoneSync] boss=%d zone=%d seq=%u state=%s pos=(%.1f, %.1f, %.1f) r=%.2f\n",
			bossId, zoneId, gimmickSeq, stateStr,
			objectPos.x, objectPos.y, objectPos.z, objectRadius);
		OutputDebugStringA(buf);
	}

	if (objectState == Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED)
	{
		if (activeZoneBarriers.find(zoneId) == activeZoneBarriers.end())
			activeZoneBarriers[zoneId] = EFFECT_MANAGER->Play(L"Barrior01", objectPos);
	}
	else if (objectState == Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED ||
		objectState == Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN)
	{
		if (auto it = activeZoneBarriers.find(zoneId); it != activeZoneBarriers.end())
		{
			EFFECT_MANAGER->Stop(it->second);
			activeZoneBarriers.erase(it);
		}
	}
}

void Scene::HandleFinalClearChoiceBegin(const Protocol::SC_FINAL_CLEAR_CHOICE_BEGIN_PACKET& choiceBegin)
{
	const uint64_t voteId = choiceBegin.voteid();

	if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
		controller->ShowHeroChoice(voteId);
}

void Scene::HandleFinalClearChoiceResult(const Protocol::SC_FINAL_CLEAR_CHOICE_RESULT_PACKET& choiceResult)
{
	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>();
	if (!controller) return;

	controller->HideHeroChoice();

	if (choiceResult.outcome() == Protocol::FINAL_CLEAR_OUTCOME_ENDING)
		controller->PlayHappyEnding();
}

void Scene::HandlePvpRoundResult(const Protocol::SC_PVP_ROUND_RESULT_PACKET& pvpResult)
{
	if (auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>())
		controller->PlayPvpEnding();
}

void Scene::HandleGameplayEffectSync(const Protocol::SC_GAMEPLAY_EFFECT_SYNC_PACKET& effectSync)
{
	const int id = NetId{ effectSync.netid() }.GetId();

	auto cit = activeCharacters.find(id);
	if (cit == activeCharacters.end())
		return;

	const XMFLOAT3 pos = cit->second->GetComponent<Transform>()->GetPosition();

	for (const auto& effect : effectSync.appliedeffects())
	{
		if (const wstring* name = BuffEffectName(effect.effectkey()))
		{
			const int handle = EFFECT_MANAGER->Play(*name, pos);
			if (handle != -1)
				activeBuffEffects[id].push_back(handle);
		}
	}
}

void Scene::UpdateBuffEffects()
{
	for (auto it = activeBuffEffects.begin(); it != activeBuffEffects.end(); )
	{
		auto cit = activeCharacters.find(it->first);
		auto& handles = it->second;

		for (auto h = handles.begin(); h != handles.end(); )
		{
			if (cit == activeCharacters.end() || !EFFECT_MANAGER->Exists(*h))
			{
				EFFECT_MANAGER->Stop(*h);
				h = handles.erase(h);
			}
			else
			{
				EFFECT_MANAGER->SetLocation(*h, cit->second->GetComponent<Transform>()->GetPosition());
				++h;
			}
		}

		if (handles.empty())
			it = activeBuffEffects.erase(it);
		else
			++it;
	}
}

const wstring* Scene::BuffEffectName(const string& effectKey) const
{
	static const wstring healing = L"Healing", attack = L"Attack Increase",
	                     maxHp = L"Max HP Increase", defense = L"Defense Increase";
	if (effectKey == "Effect.KillBuff.LifeDrain")  return &healing;		
	if (effectKey == "Effect.KillBuff.PowerSurge") return &attack;		
	if (effectKey == "Effect.KillBuff.IronWill")   return &maxHp;		
	if (effectKey == "Effect.KillBuff.SteelSkin")  return &defense;		
	return nullptr;
}

void Scene::HandleBeaconCinematicStart(const Protocol::SC_BEACON_CINEMATIC_START_PACKET& cinematicStart)
{
	OnBeaconCinematicStart();
}

void Scene::HandleTitleReplication(const Protocol::SC_TITLE_REPLICATION_PACKET& title)
{
	const int ownerId = NetId{ title.ownernetid() }.GetId();
	const int titleId = static_cast<int>(title.titleid());

	if (auto it = activeTitleEffects.find(ownerId); it != activeTitleEffects.end())
	{
		EFFECT_MANAGER->Stop(it->second);
		activeTitleEffects.erase(it);
	}

	if (titleId == 0)	
		titleIdByOwner.erase(ownerId);
	else
		titleIdByOwner[ownerId] = titleId;
}

void Scene::UpdateTitleEffects()
{
	for (auto it = titleIdByOwner.begin(); it != titleIdByOwner.end(); )
	{
		const int ownerId = it->first;

		auto cit = activeCharacters.find(ownerId);
		if (cit == activeCharacters.end())	
		{
			if (auto hit = activeTitleEffects.find(ownerId); hit != activeTitleEffects.end())
			{
				EFFECT_MANAGER->Stop(hit->second);
				activeTitleEffects.erase(hit);
			}
			it = titleIdByOwner.erase(it);
			continue;
		}

		const XMFLOAT3 pos = cit->second->GetComponent<Transform>()->GetPosition();
		auto hit = activeTitleEffects.find(ownerId);

		if (hit == activeTitleEffects.end() || !EFFECT_MANAGER->Exists(hit->second))
		{
			if (const wstring* name = TitleEffectName(it->second))
				activeTitleEffects[ownerId] = EFFECT_MANAGER->Play(*name, pos);
		}
		else
		{
			EFFECT_MANAGER->SetLocation(hit->second, pos);
		}
		++it;
	}
}

const wstring* Scene::TitleEffectName(int titleId) const
{
	static const wstring t1 = L"Title1", t2 = L"Title2", t3 = L"Title3", t4 = L"Title4",
						 t5 = L"Title5", t6 = L"Title6", t7 = L"Title7", t8 = L"Title8";
	switch (titleId)
	{
	case 1: return &t1;
	case 2: return &t2;
	case 3: return &t3;
	case 4: return &t4;
	case 5: return &t5;
	case 6: return &t6;
	case 7: return &t7;
	case 8: return &t8;
	default: return nullptr;	
	}
}
