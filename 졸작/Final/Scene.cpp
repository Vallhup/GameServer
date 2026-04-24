#include "pch.h"
#include "Scene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Material.h"
#include "Camera.h"
#include "Input.h"
#include "Animator.h"
#include "AnimationMachine.h"
#include "AnimationSetFactory.h"
#include "TrailComponent.h"
#include "FootDustComponent.h"
#include "ParrySparkComponent.h"

#include "NetHelper.h"

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

void Scene::RenderDeferred()
{
    RenderSceneDeferred();
}

void Scene::RenderForward()
{
    RenderSceneForward();
}

void Scene::RenderShadow()
{
    RenderSceneShadow();
}

void Scene::RenderEffects()
{
    RenderSceneEffects();
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

shared_ptr<GameObject> Scene::CreateMonsterObject(
	const wstring& meshPath,
	shared_ptr<AnimationSet> (*animFactory)(),
	bool twoSided)
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

void Scene::CreateBossObject(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		auto boss = CreateMonsterObject(
			L"../Assets/FBXModel/Boss/boss",
			&AnimationSetFactory::CreateFinalBossSet,
			false);
		boss->GetComponent<Transform>()->SetInitPosition(position);
		monsterPools[MonsterType::Boss].push_back(boss);
		AddGameObject(boss);
	}
}

void Scene::CreateImpObject(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		auto imp = CreateMonsterObject(
			L"../Assets/FBXModel/Monster/Imp/monster_Imp",
			&AnimationSetFactory::CreateImpSet);
		imp->GetComponent<Transform>()->SetInitPosition(position);
		monsterPools[MonsterType::Imp].push_back(imp);
		AddGameObject(imp);
	}
}

void Scene::CreateDemonStrikerObject(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		auto striker = CreateMonsterObject(
			L"../Assets/FBXModel/Monster/DemonStriker/monster_DemonStriker",
			&AnimationSetFactory::CreateDemonStrikerSet);
		striker->GetComponent<Transform>()->SetInitPosition(position);
		monsterPools[MonsterType::DemonStriker].push_back(striker);
		AddGameObject(striker);
	}
}

void Scene::CreateDemonExecutionerObject(const XMFLOAT3& position, int count)
{
	for (int i = 0; i < count; ++i)
	{
		auto executioner = CreateMonsterObject(
			L"../Assets/FBXModel/Monster/DemonExecutioner/monster_DemonExecutioner",
			&AnimationSetFactory::CreateDemonExecutionerSet);
		executioner->GetComponent<Transform>()->SetInitPosition(position);
		monsterPools[MonsterType::DemonExecutioner].push_back(executioner);
		AddGameObject(executioner);
	}
}

void Scene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}
