#include "pch.h"
#include "FirstBattleScene.h"
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
#include "TrailRenderer.h"

#include "NetId.h"
#include "NetHelper.h"

void FirstBattleScene::CreateKnightPool()
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

void FirstBattleScene::CreateBossObject()
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

void FirstBattleScene::CreateMap()
{
// Village 맵 전용
#pragma region Initialize VillageMap Elements

// 새거 1023 1023 159.4766 513x513
#pragma region Initialize Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"../Assets/FBXModel/VillageMap/ground", L"../Assets/FBXModel/VillageMap/villageTerrain.raw", 513, 1023.0f, 159.4766f, 2.0f);
#pragma endregion

#pragma region Initialize Ocean Floor
	oceanFloor = make_shared<Terrain>();
	oceanFloor->Initialize(*coreRef, L"textures/OceanFloor", L"../Assets/FBXModel/VillageMap/oceanFloorTerrain.raw", 513, 1946.701f, 170.3121f, 2.0f);
	oceanFloor->SetPosition(-903.851200f, 32.799990f, 160.528700f);
#pragma endregion

#pragma region Initialize Water
	water = make_shared<Water>();
	water->Initialize(*coreRef);
	water->SetPosition(144.0472f, 46.79999f, 939.9999f);
	water->SetScale(1500.0f, 1.0f, 2546.25f);
#pragma endregion

#pragma endregion
}

void FirstBattleScene::CreateEffectSamples()
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

float FirstBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

void FirstBattleScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void FirstBattleScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
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

void FirstBattleScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
{
	NetId nid{ move.netid() };
	int id = nid.GetId();
	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		auto transform = it->second->GetComponent<Transform>();
		const XMFLOAT3& pos = transform->GetPosition();

		transform->SetPosition(move.x(), SampleHeightAt(move.x(), move.z())/*move.y()*/, move.z());
		transform->SetTargetRotation(move.yaw());
	}
}

void FirstBattleScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void FirstBattleScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void FirstBattleScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Village);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}

shared_ptr<MainCharacter> FirstBattleScene::GetAvailableKnight() const
{
	for (auto& knight : knightPool)
	{
		if (knight->GetId() == -1)
			return knight;
	}

	return nullptr;
}

shared_ptr<MainCharacter> FirstBattleScene::GetMyPlayer() const
{
	if (myPlayer)
		return myPlayer;

	return nullptr;
}

void FirstBattleScene::Release()
{
}

void FirstBattleScene::Reset()
{
	//----
	// 임시 코드임, First->Second 연결 해보려고 시도하는 코드임
	sManagerRef->SetSharedKnight(myPlayer);
	sManagerRef->SetSharedBoss(bossObject);
	//----

	instancingBatches.clear();
	knightPool.clear();
	activeCharacters.clear();
	myPlayer = nullptr;
	bossObject = nullptr;
	gameObjects.clear();

	if (trailRenderer)
		trailRenderer->Clear();

	OutputDebugStringA("SoloGameScene Data has been deleted!! \n----------------------------------------\n");
}

void FirstBattleScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void FirstBattleScene::InitializeSceneObjectPools()
{
}

void FirstBattleScene::InitializeLogic()
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

	// Trail Renderer 초기화
	trailRenderer = make_unique<TrailRenderer>();
	trailRenderer->Initialize(coreRef->GetDevice(), 32);
	trailRenderer->SetColor({ 1.0f, 0.6f, 0.2f, 1.0f });	// 주황빛 검기
	trailRenderer->SetLifetime(0.13f);

	OutputDebugStringA("CSLoginPacket has sent!!\n");
}

void FirstBattleScene::UpdateScene(const float deltaTime)
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

	if (water)
		water->Update(deltaTime);

	// Trail 업데이트
	if (trailRenderer && myPlayer)
	{
		auto animMachine = myPlayer->GetComponent<AnimationMachine>();
		bool isAttacking = animMachine && animMachine->IsPlaying("Attack");

		if (isAttacking && !trailRenderer->IsActive())
		{
			trailRenderer->SetActive(true);
		}
		else if (!isAttacking && trailRenderer->IsActive())
		{
			trailRenderer->SetActive(false);
		}

		// 공격 중이면 칼 위치 추적하여 트레일 포인트 추가
		if (trailRenderer->IsActive())
		{
			auto animator = myPlayer->GetComponent<Animator>();
			auto transform = myPlayer->GetComponent<Transform>();

			if (animator && animator->IsInitialized())
			{
				// 본 45 = 무기/손 본
				XMFLOAT3 bonePos = animator->GetBonePosition(45);
				XMVECTOR boneRotQuat = animator->GetBoneRotation(45);

				XMMATRIX worldMat = transform->GetWorldMatrix();
				XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

				// Z축 90도 오프셋 회전 (Effekseer와 동일)
				XMVECTOR offsetRot = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2);

				// 플레이어 회전
				XMFLOAT3 playerRot = transform->GetRotation();
				XMVECTOR playerRotQuat = XMQuaternionRotationRollPitchYaw(playerRot.x, playerRot.y, playerRot.z);

				// 회전 순서: 오프셋 → 뼈 회전 → 플레이어 회전
				XMVECTOR finalRotQuat = XMQuaternionMultiply(offsetRot, boneRotQuat);
				finalRotQuat = XMQuaternionMultiply(finalRotQuat, playerRotQuat);
				XMMATRIX rotMat = XMMatrixRotationQuaternion(finalRotQuat);

				// 칼 방향 벡터 (로컬 Y축)
				XMVECTOR swordDir = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
				swordDir = XMVector3Normalize(swordDir);

				// 칼 끝과 손잡이 위치 계산
				float bladeLength = 1.0f;
				XMVECTOR topPos = XMVectorAdd(worldPos, XMVectorScale(swordDir, bladeLength));
				XMVECTOR bottomPos = worldPos;

				XMFLOAT3 top, bottom;
				XMStoreFloat3(&top, topPos);
				XMStoreFloat3(&bottom, bottomPos);

				trailRenderer->AddPoint(top, bottom);
			}
		}

		trailRenderer->Update(deltaTime);
	}

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

void FirstBattleScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());
	//renderer->RenderCollisionMeshWireframe(*coreRef, gameObjects);

	// Render terrain
	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

	// Render oceanFloor
	if (oceanFloor)
		renderer->RenderTerrain(*coreRef, oceanFloor.get());

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

void FirstBattleScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());

	if (water)
		renderer->RenderWater(*coreRef, water.get());

	renderer->RenderForward(*coreRef, gameObjects, cam.get());
}

void FirstBattleScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
}

void FirstBattleScene::RenderSceneEffects()
{
	if (cam)
		EFFECT_MANAGER->Render(*coreRef, cam.get());

	// Trail 렌더링
	if (trailRenderer)
		trailRenderer->Render(*coreRef);
}

void FirstBattleScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestLoadingScene(SceneType::Castle);
	}
}
