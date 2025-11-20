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
	int j = 0;

	for (int i = 0; i < MAX_KNIGHT_COUNT; ++i)
	{
		j = i / 10;
		auto knight = make_shared<MainCharacter>();
		knight->SetId(-1);
		auto meshRenderer = knight->AddComponent<MeshRenderer>();
		auto transform = knight->AddComponent<Transform>();
		auto animator = knight->AddComponent<Animator>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");
		transform->SetInitPosition(-5.f + (1.f * (i % 10)), 0.f, 5.f - (1.f *j));
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
	transform->SetInitPosition(5.f, 0.f, -5.f);
	transform->SetRotation(0.f, 0.f, 0.f);
	transform->SetScale(0.1f, 0.1f, 0.1f);
	AddGameObject(dragon);

	OutputDebugStringA("Dragon created!!\n");
}

void GameScene::CreateCastle()
{
	vector<wstring> names = { L"candle", L"statue1", L"statue2", L"statue3", L"throne" };
	for (int i = 2; i < 25; ++i)
	{
		auto map = make_shared<GameObject>();
		map->SetId(0);		// Id를 -1로 설정하면 지금 구조에선 렌더링 막아놓음
		auto meshRenderer = map->AddComponent<MeshRenderer>();
		auto transform = map->AddComponent<Transform>();
		if (i < 10)
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/mesh_0" + to_wstring(i));
		else if (i < 20)
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/mesh_" + to_wstring(i));
		else
			meshRenderer->SetMesh(*coreRef, L"../FBXOutput/mesh_" + names[i - 20]);
		transform->SetInitPosition(0.0f, 0.0f, 0.0f);
		transform->SetRotation(0.0f, 0.0f, 0.0f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(map);

		OutputDebugStringA("castle created!!\n");
	}
}

void GameScene::CreatePillars()
{
	vector<XMFLOAT3> pillarpos = {
		{5.62315f, -0.016064f, -9.59924f  },
		{16.504f, -0.016064f, -9.59924f   },
		{-16.5107f, -0.016064f, -9.59924f},
		{-5.62991f, -0.016064f, -9.59924f},
		{5.62315f, -0.016064f, 0.831307f   },
		{16.504f, -0.016064f, 0.831307f	   },
		{-16.5107f, -0.016064f, 0.831307f },
		{-5.62991f, -0.016064f, 0.831307f },
		{5.62315f, -0.016064f, 11.2619f	   },
		{16.504f, -0.016064f, 11.2619f	   },
		{-16.5107f, -0.016064f, 11.2619f  },
		{-5.62991f, -0.016064f, 11.2619f  },
		{5.62315f, -0.016064f, 21.8516f	   },
		{16.504f, -0.016064f, 21.8516f	   },
		{-16.5107f, -0.016064f, 21.8516f  },
		{-5.62991f, -0.016064f, 21.8516f  },
		{5.62315f, -0.016064f, 32.2828f	   },
		{16.504f, -0.016064f, 32.2828f	   },
		{-16.5107f, -0.016064f, 32.2828f  },
		{-5.62991f, -0.016064f, 32.2828f  },
		{26.9762f, -0.016064f, -9.59924f  },
		{-26.9815f, -0.016064f, -9.59924f},
		{5.62315f, -0.016064f, -35.3441f  },
		{16.504f, -0.016064f, -35.3441f   },
		{-16.5107f, -0.016064f, -35.3441f},
		{-5.62991f, -0.016064f, -35.3441f},
		{26.9762f, -0.016064f, -35.3441f  },
		{-26.9815f, -0.016064f, -35.3441f},
		{5.62315f, -0.016064f, -45.8333f  },
		{16.504f, -0.016064f, -45.8333f   },
		{-16.5107f, -0.016064f, -45.8333f},
		{-5.62991f, -0.016064f, -45.8333f},
		{-34.657f, -0.016064f, -27.727f  },
		{-34.657f, -0.016064f, -17.2682f },
		{34.5867f, -0.016064f, -27.727f   },
		{34.5867f, -0.016064f, -17.2682f  }
	};

	for (int i = 0; i < 36; ++i)
	{
		auto pillar = make_shared<GameObject>();
		pillar->SetId(0);
		auto meshRenderer = pillar->AddComponent<MeshRenderer>();
		auto transform = pillar->AddComponent<Transform>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/mesh_pillar");
		transform->SetInitPosition(pillarpos[i].x, pillarpos[i].y, pillarpos[i].z);
		transform->SetRotation(0.0f, 0.0f, 0.0f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(pillar);

		OutputDebugStringA(("pillar " + to_string(i) + " has created!!\n").c_str());
	}
}

void GameScene::CreateFloor()
{
	vector<XMFLOAT3> floorpos = {
		{-11.0406f, 0.0f, 27.0283f	 },
		{-0.172962f, 0.0f, 27.0283f	 },
		{10.6946f, 0.0f, 27.0283f		 },
		{-11.0406f, 0.0f, 16.5593f	 },
		{-0.172962f, 0.0f, 16.5593f	 },
		{10.6946f, 0.0f, 16.5593f		 },
		{-11.0406f, 0.0f, 6.09022f	 },
		{-0.172962f, 0.0f, 6.09022f	 },
		{10.6946f, 0.0f, 6.09022f		 },
		{-11.0406f, 0.0f, -4.37883f	 },
		{-0.172962f, 0.0f, -4.37883f	 },
		{10.6946f, 0.0f, -4.37883f	 },
		{-11.0406f, 0.0f, -14.8479f	 },
		{-0.172962f, 0.0f, -14.8479f },
		{10.6946f, 0.0f, -14.8479f	 },
		{-11.0406f, 0.0f, -25.3169f	 },
		{-0.172962f, 0.0f, -25.3169f},
		{10.6946f, 0.0f, -25.3169f	 },
		{-11.0406f, 0.0f, -35.7861f	 },
		{-0.172962f, 0.0f, -35.7861f	 },
		{10.6946f, 0.0f, -35.7861f	 },
		{-11.0406f, 0.0f, -46.2553f	 },
		{-0.172962f, 0.0f, -46.2553f	 },
		{10.6946f, 0.0f, -46.2553f	 },
		{-32.7753f, 0.0f, -14.8479f	 },
		{-21.9077f, 0.0f, -14.8479f	 },
		{-32.7753f, 0.0f, -25.3169f	 },
		{-21.9077f, 0.0f, -25.3169f	 },
		{-32.7753f, 0.0f, -35.7861f	 },
		{-21.9077f, 0.0f, -35.7861f	 },
		{21.561f, 0.0f, -14.8479f		 },
		{32.4286f, 0.0f, -14.8479f	 },
		{21.561f, 0.0f, -25.3169f		 },
		{32.4286f, 0.0f, -25.3169f	 },
		{21.561f, 0.0f, -35.7861f		 },
		{32.4286f, 0.0f, -35.7861f	 }
	};

	for (int i = 0; i < 36; ++i)
	{
		auto floor = make_shared<GameObject>();
		floor->SetId(0);
		auto meshRenderer = floor->AddComponent<MeshRenderer>();
		auto transform = floor->AddComponent<Transform>();
		meshRenderer->SetMesh(*coreRef, L"../FBXOutput/mesh_01");
		transform->SetInitPosition(floorpos[i].x, floorpos[i].y, floorpos[i].z);
		transform->SetRotation(0.0f, 0.0f, 0.0f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		AddGameObject(floor);

		OutputDebugStringA(("floor " + to_string(i) + " has created!!\n").c_str());
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
		{u"Aura01_HDR2", 1.f, 0.f, 0.5f},
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

void GameScene::InitializeSceneObjectPools()
{
}

void GameScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nGameScene Data has been created!! \n");

	CreateKnightPool();
	CreateDragon();
	CreateCastle();
	CreatePillars();
	CreateFloor();
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

	if (effectObjects[2] && myPlayer) {
		if (auto transform = effectObjects[2]->GetComponent<Transform>())
		{
			XMFLOAT3 pos = myPlayer->GetComponent<Transform>()->GetPosition();
			transform->SetInitPosition(pos.x, pos.y, pos.z);
		}
	}

	if (myPlayer)	// 그림자 반경을 플레이어 기준으로 움직이는거 테스트 위한 임시 코드임
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());
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

void GameScene::RenderSceneShadow()
{
	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
			meshRenderer->RenderShadow(*coreRef);
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
