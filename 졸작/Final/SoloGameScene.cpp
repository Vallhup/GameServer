#include "pch.h"
#include "SoloGameScene.h"
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
#include "Water.h"
#include "EffectRenderer.h"
#include "EffectManager.h"

#include "UIManager.h"
#include "GameSceneUIController.h"

#include "NetId.h"
#include "NetHelper.h"

SoloGameScene::~SoloGameScene() = default;

void SoloGameScene::CreateKnightPool()
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

void SoloGameScene::CreateBossObject()
{
	bossObject = make_shared<GameObject>();
	bossObject->SetId(-1);
	auto mesh = bossObject->AddComponent<Mesh>();
	auto transform = bossObject->AddComponent<Transform>();
	auto animator = bossObject->AddComponent<Animator>();
	auto animMachine = bossObject->AddComponent<AnimationMachine>();
	mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Boss/boss");

	animMachine->SetAnimationSet(AnimationSetFactory::CreateFinalBossSet());
	transform->SetInitPosition(22.f, SampleHeightAt(22.0f, 22.0f), 22.f);
	transform->SetRotation(0.f, 3.14f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);
	AddGameObject(bossObject);
}

void SoloGameScene::CreateMap()
{
// 이걸 여기서 한번 더 호출할 경우엔, 어떻게 되냐면 이제 똑같은 위치에 똑같은 맵 에셋을 두번 그리게 되는거임
// 그렇게 하면 프레임이 당연히 안나오겟죠 ?
//#pragma region Initialize Map Elements
//	InstanceLoader mapLoader;
//	mapLoader.Load(L"../Assets/FBXModel/Map/MapInstanceData.txt");
//
//	int count = 0;
//	// during count 50 ~ 60 rapid lower fps range
//	for (const auto& [modelName, instanceData] : mapLoader.GetAllData()) {
//		if (instanceData.empty())  
//			continue;
//
//		wstring path = L"../Assets/FBXModel/Map/" + wstring(modelName.begin(), modelName.end());
//
//		if (!filesystem::exists(path + L"_0.mesh"))
//			continue;
//
//		CreateAndBatchObjects(path, instanceData, instancingBatches);
//	}
//	//OutputDebugStringA(("Map data count: " + to_string(count) + '\n').c_str());
//#pragma endregion

	// 새거 1023 1023 159.4766 513x513
#pragma region Initialize Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"../Assets/FBXModel/VillageMap/ground", L"../Assets/FBXModel/VillageMap/terrain.raw", 513, 1023.0f, 159.4766f);
#pragma endregion

#pragma region Initialize Water
	water = make_shared<Water>();
	water->Initialize(*coreRef);
	water->SetPosition(0.0f, 41.0f, 0.0f);
	water->SetScale(10.0f);
#pragma endregion

//#pragma region Initialize VillageMap Elements
//	InstanceLoader mapLoader;
//	mapLoader.Load(L"../Assets/FBXModel/VillageMap/MapInstanceData.txt");
//
//	for (const auto& [modelName, instanceData] : mapLoader.GetAllData()) {
//		if (instanceData.empty())
//			continue;
//
//		wstring path = L"../Assets/FBXModel/VillageMap/" + wstring(modelName.begin(), modelName.end());
//
//		if (!filesystem::exists(path + L"_0.mesh"))
//			continue;
//
//		CreateAndBatchObjects(path, instanceData, instancingBatches);
//	}
//#pragma endregion

	/*constexpr InstanceData testData = {
		{ 14.0f, 4.0f, 14.0f }, { 0.0f, 0.0f, 0.0f}, { 1.0f, 1.0f, 1.0f }, false, 25.0f
	};

	AddGameObject(CreateStaticMesh(L"../Assets/FBXModel/VillageMap/SM_Wall_04", testData));*/
}

void SoloGameScene::CreateEffectSamples()
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
		{u"Dissolve", 2.f, 0.f, 0.f},
		{u"SwordThunder", 0.0f, 0.0f, 0.0f}
	};

	for (int i = 0; i < info.size(); ++i)
	{
		wstring name(info[i].name.begin(), info[i].name.end());
		EFFECT_MANAGER->PreLoad(name);

		auto effectSample = make_shared<GameObject>();
		auto effectRenderer = effectSample->AddComponent<EffectRenderer>();
		auto transform = effectSample->AddComponent<Transform>();

		effectRenderer->SetEffectName(name);

		transform->SetInitPosition(info[i].x, info[i].y, info[i].z);
		effectObjects.push_back(effectSample);
		AddGameObject(effectSample);
	}
}

float SoloGameScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

void SoloGameScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void SoloGameScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
{
	NetId nid{ add.netid() };
	int id = nid.GetId();
	int type = add.typeid_();

	if (type == 5) // Final_Boss
	{
		if (bossObject)
		{
			bossObject->SetId(id);
			auto transform = bossObject->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = bossObject;
		}
	}
	else if (type == 1) // Knight
	{
		auto player = GetAvailableKnight();
		if (player)
		{
			player->SetId(id);
			auto transform = player->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
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

void SoloGameScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
{
	NetId nid{ move.netid() };
	int id = nid.GetId();
	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		auto transform = it->second->GetComponent<Transform>();
		const XMFLOAT3& pos = transform->GetPosition();

		transform->SetPosition(move.x(), move.y(), move.z());
		transform->SetTargetRotation(move.yaw());
	}
}

void SoloGameScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void SoloGameScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void SoloGameScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::MainGame);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}

shared_ptr<MainCharacter> SoloGameScene::GetAvailableKnight() const
{
	for (auto& knight : knightPool)
	{
		if (knight->GetId() == -1)
			return knight;
	}

	return nullptr;
}

shared_ptr<MainCharacter> SoloGameScene::GetMyPlayer() const
{
	if (myPlayer)
		return myPlayer;

	return nullptr;
}

void SoloGameScene::Release()
{
}

void SoloGameScene::Reset()
{
	instancingBatches.clear();
	knightPool.clear();
	activeCharacters.clear();
	myPlayer = nullptr;
	bossObject = nullptr;
	gameObjects.clear();

	OutputDebugStringA("SoloGameScene Data has been deleted!! \n----------------------------------------\n");
}

void SoloGameScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void SoloGameScene::InitializeSceneObjectPools()
{
}

void SoloGameScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nSoloGameScene Data has been created!! \n");

	CreateKnightPool();

	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList());
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());

	CreateMap();
	CreateBossObject();
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

	SetNetworkManager(NETWORK_MANAGER);

	{
		Protocol::CS_LOGIN_PACKET login;
		SendBuffer* data = PacketFactory::Serialize<Protocol::CS_LOGIN_PACKET>(
			PacketType::CS_LOGIN, login);
		_nManager->Send(data);
	}

	OutputDebugStringA("CSLoginPacket has sent!!\n");
}

void SoloGameScene::UpdateScene(const float deltaTime)
{
	/*if (effectObjects.size() > 0 && INPUT.GetKeyDown('1'))
		effectObjects[0]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 1 && INPUT.GetKeyDown('2'))
		effectObjects[1]->GetComponent<EffectRenderer>()->PlayEffect();*/

	if (effectObjects.size() > 2 && INPUT.GetKeyDown('3'))
		effectObjects[2]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 3 && INPUT.GetKeyDown('4'))
		effectObjects[3]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 4 && INPUT.GetKeyDown('5'))
		effectObjects[4]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 5 && INPUT.GetKeyDown('6')) {
		effectObjects[5]->GetComponent<EffectRenderer>()->PlayEffect();
		effectObjects[6]->GetComponent<EffectRenderer>()->PlayEffect();
	}

	if (effectObjects.size() > 6 && INPUT.GetKeyDown('7'))
		effectObjects[7]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 7 && INPUT.GetKeyDown('8'))
		effectObjects[8]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects[8] && myPlayer) {
		if (auto effectRenderer = effectObjects[8]->GetComponent<EffectRenderer>())
		{
			auto animator = myPlayer->GetComponent<Animator>();
			auto transform = myPlayer->GetComponent<Transform>();

			if (!animator || !animator->IsInitialized()) return;

			XMFLOAT3 bonePos = animator->GetBonePosition(45);
			XMVECTOR boneRot = animator->GetBoneRotation(45);

			XMMATRIX worldMat = transform->GetWorldMatrix();
			XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

			// z축 90도 초기 회전 - DirectX12와 effekseer 축 차이인듯?
			XMVECTOR offsetRot = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2);

			// 뼈 회전 × 플레이어 회전
			XMFLOAT3 playerRot = transform->GetRotation();
			XMVECTOR playerRotQuat = XMQuaternionRotationRollPitchYaw(playerRot.x, playerRot.y, playerRot.z);

			// 초기 회전 → 뼈 회전 → 플레이어 회전
			XMVECTOR finalRot = XMQuaternionMultiply(offsetRot, boneRot);
			finalRot = XMQuaternionMultiply(finalRot, playerRotQuat);

			XMMATRIX finalMat = XMMatrixRotationQuaternion(finalRot) * XMMatrixTranslationFromVector(worldPos);
			effectRenderer->SetWorldMatrix(finalMat);
		}
	}

	if (myPlayer)	// Temporary Code for Player Centered Shadow Mapping
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());

		SoundManager* sound = SOUND_MANAGER;

		/*if (transform->GetPosition().z < -11.0f)
		{
			sound->PlayBGM("../Assets/Music/BGM/background.mp3");
		}
		else
		{
			if (INPUT.GetKeyDown('0'))
				sound->StopBGM();
		}*/

		if (INPUT.GetKeyDown('0'))
		{
			XMFLOAT3 pos = myPlayer->GetComponent<Transform>()->GetPosition();
			OutputDebugStringA(("MyPlayer Pos: " + to_string(pos.x) + ", " + to_string(pos.y) + ", " + to_string(pos.z) + "\n").c_str());
		}

		if (INPUT.GetKeyDown('2'))
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
	/*for (const auto& [sessionId, player] : activeCharacters)
	{
		if (auto transform = player->GetComponent<Transform>())
		{
			const XMFLOAT3& pos = transform->GetPosition();
			float terrainHeight = SampleHeightAt(pos.x, pos.z);
			transform->SetHeightImmediate(terrainHeight);
		}
	}*/

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, instancingBatches, myPlayer);
	
	BoundingFrustum frustum = cam->GetViewFrustum();
	XMFLOAT3 camPos = cam->GetPosition();
	XMVECTOR camPosVec = XMLoadFloat3(&camPos);

	/*auto start = chrono::high_resolution_clock::now();*/
	for (auto& batch : instancingBatches)
		batch->Update(frustum, camPosVec);
	//auto end = chrono::high_resolution_clock::now();
	//auto ms = chrono::duration_cast<chrono::microseconds>(end - start).count();
	//OutputDebugStringA(("Update: " + to_string(ms) + "us\n").c_str());
}

void SoloGameScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());
	//renderer->RenderCollisionMeshWireframe(*coreRef, gameObjects);

	// Render terrain
	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

	// Render water
	if (water)
		renderer->RenderWater(*coreRef, water.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}

	static bool hitOn = false;

	if (INPUT.GetKeyDown('1'))
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

void SoloGameScene::RenderSceneForward()
{
	skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());

	sManagerRef->GetSceneRenderer()->RenderForward(*coreRef, gameObjects, cam.get());
}

void SoloGameScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
}

void SoloGameScene::RenderSceneEffects()
{
	if (cam)
		EFFECT_MANAGER->Render(*coreRef, cam.get());
}

void SoloGameScene::RequestSceneChange()
{

}
