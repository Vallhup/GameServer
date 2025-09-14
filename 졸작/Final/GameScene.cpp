#include "pch.h"
#include "GameScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameObject.h"
#include "MainCharacter.h"
#include "MeshRenderer.h"
#include "Transform.h"
#include "Animator.h"
#include "Material.h"
#include "Camera.h"
#include "EffectRenderer.h"
#include "Engine.h"
#include "NetworkManager.h"

GameScene::~GameScene() = default;

void GameScene::CreateKnightPool()
{
	for (int i = 0; i < MAX_KNIGHT_COUNT; ++i)
	{
		auto knight = make_shared<MainCharacter>();
		knight->SetId(-1);
		auto meshRenderer = knight->AddComponent<MeshRenderer>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");
		transform->SetInitPosition((1.f * i), 0.f, 5.f);
		transform->SetRotation(-1.57f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		knightPool.push_back(knight);
		AddGameObject(knight);
	}
}

void GameScene::CreateDragon()
{
	dragon = make_shared<GameObject>();
	dragon->SetId(-1);		// Id를 -1로 설정하면 지금 구조에선 렌더링 막아놓음
	auto meshRenderer = dragon->AddComponent<MeshRenderer>();
	auto transform = dragon->AddComponent<Transform>();
	auto animator = dragon->AddComponent<Animator>();
	meshRenderer->SetMesh(*coreRef, L"../FBXOutput/Dragon");
	transform->SetPosition(5.f, 0.f, -5.f);
	transform->SetRotation(0.f, 0.f, 0.f);
	transform->SetScale(0.1f, 0.1f, 0.1f);
	AddGameObject(dragon);

	OutputDebugStringA("Dragon created!!\n");
}

void GameScene::CreateCastle()
{
	vector<wstring> names = { L"bookshelf", L"candle", L"chair", L"pillar", L"statue1", L"statue2", L"statue3", L"table", L"throne" };
	for (int i = 1; i < 29; ++i)
	{
		auto map = make_shared<GameObject>();
		map->SetId(0);		// Id를 -1로 설정하면 지금 구조에선 렌더링 막아놓음
		auto meshRenderer = map->AddComponent<MeshRenderer>();
		auto transform = map->AddComponent<Transform>();
		if (i < 10)
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/map_cathedral_0" + to_wstring(i));
		else if (i < 20)
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/map_cathedral_" + to_wstring(i));
		else
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/map_cathedral_" + names[i - 20]);
		transform->SetInitPosition(0.0f, 0.0f, 0.0f);
		transform->SetRotation(0.0f, 0.0f, 0.0f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(map);

		OutputDebugStringA("Strut created!!\n");
	}
}

void GameScene::CreateEffectSamples()
{
	struct EffectInfo {
		u16string name;
		float x;
		float y;
		float z;
	};

	vector<EffectInfo> info = {
		{u"Fireworks", 1.f, 0.f, -10.5f},
		{u"BloodLance", 1.f, 0.f, 0.5f},
		{u"Aura01_HDR", 1.f, 0.f, 0.5f},
		{u"Benediction", 1.f, 10.f, -10.5f},
		{u"Atmosphere", 1.f, 10.f, -10.5f},
		{u"CandleFire4", 27.f, 29.f, -70.0f},
		{u"CandleFire4", -27.f, 29.f, -70.0f},
		{u"CandleFire3", 7.2f, 3.25f, -4.2f},
	};

	for (int i = 0; i < info.size(); ++i)
	{
		auto effectSample = make_shared<GameObject>();
		auto effectRenderer = effectSample->AddComponent<EffectRenderer>();
		auto transform = effectSample->AddComponent<Transform>();
		effectRenderer->Initialize(*coreRef);
		u16string path = u"../Effects/" + info[i].name + u".efk";
		effectRenderer->LoadEffect(path.c_str());
		transform->SetInitPosition(info[i].x, info[i].y, info[i].z);
		effectObjects.push_back(effectSample);
		AddGameObject(effectSample);
	}
}

shared_ptr<MainCharacter> GameScene::GetAvailableKnight() const
{
	for (auto& knight : knightPool)
	{
		if (knight->GetId() == -1)
			return knight;
	}

	return nullptr;
}

void GameScene::Release()
{
}

void GameScene::Reset()
{
	// TODO: 씬 데이터 리셋 코드 추가
	dragon.reset();
	knightPool.clear();
	activePlayers.clear();
	myPlayer = nullptr;
	gameObjects.clear();

	Material::Cleanup();
	OutputDebugStringA("GameScene Data has been deleted!! \n----------------------------------------\n");
}

void GameScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void GameScene::HandlePacket(const Protocol::GamePacket& packet)
{
	const auto& header = packet.header();
	int sessionId = header.sessionid();

	switch (header.type()) {
		case Protocol::PacketType::SC_LOGIN: {
			OutputDebugStringA("SC_LOGIN packet received\n");
			Protocol::SC_LOGIN_PACKET login;
			if (login.ParseFromArray(packet.body().data(), packet.body().size())) {
				GET(Input).SetClientID(sessionId);
				OutputDebugStringA(("My Session ID: " + to_string(GET(Input).GetClientID()) + "\n").c_str());
			}
			break;
		}
		case Protocol::PacketType::SC_ADD: {
			OutputDebugStringA("SC_ADD packet received\n");
			Protocol::SC_ADD_PACKET add;
			if (add.ParseFromArray(packet.body().data(), packet.body().size())) {
				Protocol::Vec3 pos = add.pos();

				auto player = GetAvailableKnight();
				if (player) {
					player->SetId(sessionId);
					auto transform = player->GetComponent<Transform>();
					transform->SetInitPosition(pos.x(), pos.y(), pos.z());

					activePlayers[sessionId] = player;
				}

				if (sessionId == GET(Input).GetClientID()) {
					myPlayer = player;
					myPlayer->SetCamera(cam.get());
					OutputDebugStringA("My character activated!\n");
				} 
			}
			break;
		}
		case Protocol::PacketType::SC_MOVE_OBJECT: {
			Protocol::SC_MOVE_PACKET move;
			if (move.ParseFromArray(packet.body().data(), packet.body().size())) {
				Protocol::Vec3 pos = move.pos();

				auto it = activePlayers.find(sessionId);
				if (it != activePlayers.end())
				{
					auto transform = it->second->GetComponent<Transform>();
					transform->SetPosition(pos.x(), pos.y(), pos.z());
					transform->SetTargetRotation(move.rot());
				}
			}
			break; 
		}
		case Protocol::PacketType::SC_REMOVE: {
			OutputDebugStringA("SC_REMOVE packet received\n");
			break;
		}
		case Protocol::PacketType::SC_ATTACK: {
			Protocol::SC_ATTACK_PACKET attack;
			if (attack.ParseFromArray(packet.body().data(), packet.body().size())) {
				if (sessionId == GET(Input).GetClientID()) {
					// TODO : Client Attack Animation 보정
					OutputDebugStringA("SC_ATTACK_PACKET received\n");
				}
			}
			break;
		}
		case Protocol::PacketType::SC_DODGE: {
			Protocol::SC_DODGE_PACKET dodge;
			if (dodge.ParseFromArray(packet.body().data(), packet.body().size())) {
				if (sessionId == GET(Input).GetClientID()) {
					// TODO : Client Dodge Animation 보정
					OutputDebugStringA("SC_DODGE_PACKET received\n");
				}
			}
		}
	}
}

const float* GameScene::GetBackgroundColor()
{
	return Colors::Snow;
}

void GameScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nGameScene Data has been created!! \n");

	CreateKnightPool();
	CreateDragon();
	CreateCastle();
	CreateEffectSamples();

	OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();
	 
	for (const auto& obj : gameObjects)
	{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->ReleaseUploadBuffers();
	}
	OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");

	SetNetworkManager(GET(Engine).GetNetworkManager());
	_nManager->Send(PacketFactory::CSLoginPacket());
	OutputDebugStringA("CSLoginPacket has sent!!\n");
}

void GameScene::UpdateScene(const float deltaTime)
{
	if (dragon) {
		auto animator = dragon->GetComponent<Animator>();
		if (animator) {
			if (GET(Input).GetKeyDown('1')) {
				animator->TransitionToAnimation(0, 0.6f);  // Fly
				OutputDebugStringA("Dragon Animation 0 (Fly) played!\n");
			}

			if (GET(Input).GetKeyDown('2')) {
				animator->TransitionToAnimation(1, 0.4f);  // Idle
				OutputDebugStringA("Dragon Animation 1 (Idle) played!\n");
			}

			if (GET(Input).GetKeyDown('3')) {
				animator->TransitionToAnimation(2, 0.4f);  // Run
				OutputDebugStringA("Dragon Animation 2 (Run) played!\n");
			}

			if (GET(Input).GetKeyDown('4')) {
				animator->TransitionToAnimation(3, 0.4f);  // Walk
				OutputDebugStringA("Dragon Animation 3 (Walk) played!\n");
			}
		}
	}

	if (effectObjects.size() > 0 && GET(Input).GetKeyDown('1'))
		effectObjects[0]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 1 && GET(Input).GetKeyDown('2'))
		effectObjects[1]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 2 && GET(Input).GetKeyDown('3'))
		effectObjects[2]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 3 && GET(Input).GetKeyDown('4'))
		effectObjects[3]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 4 && GET(Input).GetKeyDown('5'))
		effectObjects[4]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 5 && GET(Input).GetKeyDown('6')) {
		effectObjects[5]->GetComponent<EffectRenderer>()->PlayEffect();
		effectObjects[6]->GetComponent<EffectRenderer>()->PlayEffect();
		effectObjects[7]->GetComponent<EffectRenderer>()->PlayEffect();
	}

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);
}

void GameScene::RenderSceneDeferred()
{
	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
			if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
				meshRenderer->RenderDeferred(*coreRef);
		}
	}
}

void GameScene::RenderSceneForward()
{
	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
			if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
				meshRenderer->RenderForward(*coreRef);
		}
	}
}

void GameScene::RenderSceneEffects()
{
	for (const auto& obj : gameObjects)
	{
		if (auto effectRenderer = obj->GetComponent<EffectRenderer>())
			effectRenderer->Render(*coreRef, cam.get());
	}
}

int GameScene::GetSceneWidth() const
{
	return 0;
}

void GameScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Scene1);
	}
}
