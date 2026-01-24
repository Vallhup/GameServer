#include "pch.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "MainCharacter.h"
#include "Animator.h"
#include "Material.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "SoundManager.h"
#include "ImGuiManager.h"
#include "VertexIndexBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "SkyBox.h"
#include "AnimationMachine.h"
#include "AnimationSetFactory.h"
#include "InstanceLoader.h"
#include "Terrain.h"
#include "EffectRenderer.h"
#include "EffectManager.h"

GameScene::~GameScene() = default;

void GameScene::CreateKnightPool()
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
		mesh->SetCollisionMesh(*coreRef, L"../Assets/FBXModel/Knight/knight6");

		animMachine->SetAnimationSet(AnimationSetFactory::CreateKnightSet());
		transform->SetInitPosition(-5.f + (1.f * (i % 10)), 0.f, 5.f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		knightPool.push_back(knight);
		AddGameObject(knight);
	}
}

void GameScene::CreateMap()
{
#pragma region Initialize Map Elements
	InstanceLoader mapLoader;
	mapLoader.Load(L"../Assets/FBXModel/Map/MapInstanceData.txt");

	int count = 0;
	for (const auto& [modelName, instanceData] : mapLoader.GetAllData()) {
		if (instanceData.empty())  
			continue;

		wstring path = L"../Assets/FBXModel/Map/" + wstring(modelName.begin(), modelName.end());

		if (!filesystem::exists(path + L"_0.mesh"))
			continue;

		CreateAndBatchObjects(path, instanceData, instancingBatches);
	}
#pragma endregion

#pragma region Initialize Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"../Assets/FBXModel/Map/ground", L"../Assets/FBXModel/Map/terrain.raw", 256, 160.0f, 600.0f);
#pragma endregion
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
		{u"CandleFire5", 14.2448f, 14.5f, -43.6773f},
		{u"CandleFire5", -14.1011f, 14.5f, -43.6773f},
		{u"Dissolve", 2.f, 0.f, 0.f}
	};

	for (int i = 0; i < info.size(); ++i)
	{
		wstring name(info[i].name.begin(), info[i].name.end());
		GET(EffectManager).PreLoad(name);

		auto effectSample = make_shared<GameObject>();
		auto effectRenderer = effectSample->AddComponent<EffectRenderer>();
		auto transform = effectSample->AddComponent<Transform>();

		effectRenderer->SetEffectName(name);

		transform->SetInitPosition(info[i].x, info[i].y, info[i].z);
		effectObjects.push_back(effectSample);
		AddGameObject(effectSample);
	}
}

float GameScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
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
	instancingBatches.clear();
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

void GameScene::HandlePacket(const PacketHeader& header, const BYTE* data)
{
	PacketType type = static_cast<PacketType>(header.type);

	switch (type) {
	case PacketType::SC_LOGIN:
	{
		OutputDebugStringA("SC_LOGIN packet received\n");
		Protocol::SC_LOGIN_PACKET login;
		if (PacketFactory::Deserialize<Protocol::SC_LOGIN_PACKET>(header, data, &login))
		{
			GET(Input).SetClientID(login.sessionid());
			OutputDebugStringA(("My Session ID: " + to_string(GET(Input).GetClientID()) + "\n").c_str());
		}
		break;
	}
	case PacketType::SC_ADD:
	{
		OutputDebugStringA("SC_ADD packet received\n");
		Protocol::SC_ADD_PACKET add;
		if (PacketFactory::Deserialize<Protocol::SC_ADD_PACKET>(header, data, &add))
		{
			int sessionId = add.sessionid();
			auto player = GetAvailableKnight();
			if (player)
			{
				player->SetId(sessionId);
				auto transform = player->GetComponent<Transform>();
				transform->SetInitPosition(add.x(), add.y() + 3.5f, add.z());

				transform->SetTargetRotation(add.yaw());

				activePlayers[sessionId] = player;
			}

			if (sessionId == GET(Input).GetClientID())
			{
				myPlayer = player;
				myPlayer->SetAsLocalPlayer(cam.get());

				GET(ImGuiManager).SetMyPlayer(myPlayer.get());

				OutputDebugStringA("My character activated!\n");
			}
		}
		break;
	}
	case PacketType::SC_MOVE_OBJECT:
	{
		Protocol::SC_MOVE_PACKET move;
		if (PacketFactory::Deserialize<Protocol::SC_MOVE_PACKET>(header, data, &move))
		{
			int sessionId = move.sessionid();
			auto it = activePlayers.find(sessionId);
			if (it != activePlayers.end())
			{
				auto transform = it->second->GetComponent<Transform>();
				const XMFLOAT3& pos = transform->GetPosition();

				// Y is updated every frame in UpdateScene based on terrain height
				transform->SetPosition(move.x(), pos.y, move.z());
				transform->SetTargetRotation(move.yaw());
			}
		}
		break;
	}
	case PacketType::SC_REMOVE:
	{
		OutputDebugStringA("SC_REMOVE packet received\n");
		break;
	}
	case PacketType::SC_ANIMATION_CHANGE:
	{
		Protocol::SC_ANIMATION_TRANSITION_PACKET anim;
		if (PacketFactory::Deserialize<Protocol::SC_ANIMATION_TRANSITION_PACKET>(header, data, &anim))
		{
			int sessionId = anim.sesssionid();
			if (sessionId == GET(Input).GetClientID())
				return;

			auto it = activePlayers.find(sessionId);
			if (it != activePlayers.end())
			{
				if (auto animMachine = it->second->GetComponent<AnimationMachine>())
				{
					uint32 serverAnimIdx = anim.curranim();
					uint32 startIdx = animMachine->GetAnimationSet()->GetStartIndex();
					string animName = animMachine->GetAnimationSet()->GetClipNameByIndex(serverAnimIdx - startIdx);

					animMachine->TryPlayClip(animName);
				}
			}
		}
		break;
	}
	//case PacketType::SC_ATTACK: {
	//	Protocol::SC_ATTACK_PACKET attack;
	//	if (attack.ParseFromArray(packet.body().data(), packet.body().size())) {
	//		if (sessionId == GET(Input).GetClientID()) {
	//			// TODO : Client Attack Animation ����
	//			OutputDebugStringA("SC_ATTACK_PACKET received\n");
	//		}
	//	}
	//	break;
	//}
	//case Protocol::PacketType::SC_DODGE: {
	//	Protocol::SC_DODGE_PACKET dodge;
	//	if (dodge.ParseFromArray(packet.body().data(), packet.body().size())) {
	//		if (sessionId == GET(Input).GetClientID()) {
	//			// TODO : Client Dodge Animation ����
	//			OutputDebugStringA("SC_DODGE_PACKET received\n");
	//		}
	//	}
	//}
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

	{
		auto boss = make_shared<GameObject>();
		boss->SetId(-1);
		auto mesh = boss->AddComponent<Mesh>();
		auto transform = boss->AddComponent<Transform>();
		auto animator = boss->AddComponent<Animator>();
		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Boss/boss");
		transform->SetInitPosition(2.f, 0.f, -5.f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.02f, 0.02f, 0.02f);
		AddGameObject(boss);
	}

	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList());

	CreateMap();

	CreateEffectSamples();

	OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();
	 
	for (const auto& obj : gameObjects)
	{
		if (auto mesh = obj->GetComponent<Mesh>())
			mesh->ReleaseUploadBuffers();
	}

	OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");

	SetNetworkManager(GET(Engine).GetNetworkManager());

	{
		Protocol::CS_LOGIN_PACKET login;
		SendBuffer* data = PacketFactory::Serialize<Protocol::CS_LOGIN_PACKET>(
			PacketType::CS_LOGIN, login);
		_nManager->Send(data);
	}

	OutputDebugStringA("CSLoginPacket has sent!!\n");
}

void GameScene::UpdateScene(const float deltaTime)
{
	/*if (effectObjects.size() > 0 && GET(Input).GetKeyDown('1'))
		effectObjects[0]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 1 && GET(Input).GetKeyDown('2'))
		effectObjects[1]->GetComponent<EffectRenderer>()->PlayEffect();*/

	if (effectObjects.size() > 2 && GET(Input).GetKeyDown('3'))
		effectObjects[2]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 3 && GET(Input).GetKeyDown('4'))
		effectObjects[3]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 4 && GET(Input).GetKeyDown('5'))
		effectObjects[4]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 5 && GET(Input).GetKeyDown('6')) {
		effectObjects[5]->GetComponent<EffectRenderer>()->PlayEffect();
		effectObjects[6]->GetComponent<EffectRenderer>()->PlayEffect();
	}

	if (effectObjects.size() > 6 && GET(Input).GetKeyDown('7'))
		effectObjects[7]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects[2] && myPlayer) {
		if (auto transform = effectObjects[2]->GetComponent<Transform>())
		{
			XMFLOAT3 pos = myPlayer->GetComponent<Transform>()->GetPosition();
			transform->SetInitPosition(pos.x, pos.y, pos.z);
		}
	}

	if (myPlayer)	// Temporary Code for Player Centered Shadow Mapping
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());

		SoundManager* sound = GET(Engine).GetSoundManager();

		/*if (transform->GetPosition().z < -11.0f)
		{
			sound->PlayBGM("../Assets/Music/BGM/background.mp3");
		}
		else
		{
			if (GET(Input).GetKeyDown('0'))
				sound->StopBGM();
		}*/

		if (GET(Input).GetKeyDown('0'))
		{
			XMFLOAT3 pos = myPlayer->GetComponent<Transform>()->GetPosition();
			OutputDebugStringA(("MyPlayer Pos: " + to_string(pos.x) + ", " + to_string(pos.y) + ", " + to_string(pos.z) + "\n").c_str());
		}

		if (GET(Input).GetKeyDown('2'))
		{
			auto mesh = myPlayer->GetComponent<Mesh>();
			mesh->ToggleCollisionMesh();
		}
	}

	for (const auto& obj : gameObjects)
	{
		if (!obj->IsStatic())
			obj->Update(deltaTime);
	}

	// Update player heights based on terrain
	for (const auto& [sessionId, player] : activePlayers)
	{
		if (auto transform = player->GetComponent<Transform>())
		{
			const XMFLOAT3& pos = transform->GetPosition();
			float terrainHeight = SampleHeightAt(pos.x, pos.z);
			transform->SetHeightImmediate(terrainHeight);
		}
	}

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, instancingBatches, myPlayer);
	
	BoundingFrustum frustum = cam->GetViewFrustum();
	for (auto& batch : instancingBatches)
		batch->Update(frustum);
}

void GameScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());
	renderer->RenderCollisionMeshWireframe(*coreRef, gameObjects);

	// Render terrain
	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}

	static bool hitOn = false;

	if (GET(Input).GetKeyDown('1'))
		hitOn = !hitOn;

	for (const auto& obj : gameObjects)
	{
		if (obj->GetId() != -1)
		{
			if (auto mesh = obj->GetComponent<Mesh>())
			{
				auto animator = obj->GetComponent<Animator>();

				if (hitOn && !animator)
					obj->RenderDebugBoundingBox(*coreRef, { 1, 0, 0, 1 });
			}
		}
	}

	for (const auto& group : instancingBatches)
	{
		const auto& batchObjects = group->GetObjects();

		for (const auto& obj : batchObjects)
		{
			if (auto mesh = obj->GetComponent<Mesh>())
			{
				auto animator = obj->GetComponent<Animator>();

				if (hitOn && !animator)
					obj->RenderDebugBoundingBox(*coreRef, { 0, 1, 1, 1 });
			}
		}
	}
}

void GameScene::RenderSceneForward()
{
	skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());

	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void GameScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
}

void GameScene::RenderSceneEffects()
{
	if (cam)
		GET(EffectManager).Render(*coreRef, cam.get());
}

void GameScene::RequestSceneChange()
{

}
