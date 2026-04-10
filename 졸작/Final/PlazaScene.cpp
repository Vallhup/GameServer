#include "pch.h"
#include "PlazaScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "MainCharacter.h"
#include "Animator.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "SoundManager.h"
#include "ImGuiManager.h"
#include "SkyBox.h"
#include "AnimationMachine.h"
#include "AnimationSetFactory.h"
#include "Terrain.h"
#include "EffectRenderer.h"
#include "EffectManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "EffectComponent.h"
#include "TrailComponent.h"
#include "FootDustComponent.h"
#include "FlameComponent.h"
#include "ParrySparkComponent.h"

#include "NetId.h"
#include "EntityId.h"
#include "NetHelper.h"

shared_ptr<MainCharacter> PlazaScene::GetAvailableKnight() const
{
	for (auto& knight : knightPool)
	{
		if (knight->GetId() == -1)
			return knight;
	}

	return nullptr;
}

shared_ptr<MainCharacter> PlazaScene::GetMyPlayer() const
{
	if (myPlayer)
		return myPlayer;

	return nullptr;
}

void PlazaScene::Release()
{
}

void PlazaScene::Reset()
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
	impObject = nullptr;
	flameObject = nullptr;
	gameObjects.clear();

	OutputDebugStringA("PlazaScene Data has been deleted!! \n----------------------------------------\n");
}

void PlazaScene::AddGameObject(shared_ptr<GameObject> obj)
{
	gameObjects.push_back(obj);
}

void PlazaScene::InitializeSceneObjectPools()
{
}

void PlazaScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nPlazaScene Data has been created!! \n");

	{
		auto _t0 = chrono::high_resolution_clock::now();
		CreateKnightPool();
		auto _t1 = chrono::high_resolution_clock::now();
		auto _ms = chrono::duration_cast<chrono::microseconds>(_t1 - _t0).count();
		OutputDebugStringA(("[Plaza] CreateKnightPool: " + to_string(_ms) + " us\n").c_str());
	}

	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList());
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());

#pragma region Initialize Plaza Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"PlazaMap/textures/plazaFloor", L"../Assets/FBXModel/PlazaMap/plazaTerrain.raw", 513, 1016.0f, 27.01563f, 1.0f);
#pragma endregion

	CreateBossObject();
	CreateImpObject();
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

	flameObject = make_shared<GameObject>();
	flameObject->SetId(-1);
	auto flame = flameObject->AddComponent<FlameComponent>();
	flame->Initialize(coreRef->GetDevice(), 32);
	flame->SetTexture(coreRef->GetDevice(), coreRef->GetGraphicsCmdList(), L"../Assets/Effects/Textures/T_candleflame.png");
	flame->SetParticleSize(0.5f);
	flame->Spawn(XMFLOAT3(484.607025f, 6.f, 481.862946f));
	AddGameObject(flameObject);

	OutputDebugStringA("CSLoginPacket has sent!!\n");
}

void PlazaScene::UpdateScene(const float deltaTime)
{
	if (effectObjects.size() > 0 && INPUT.GetKeyDown('1'))
		effectObjects[0]->GetComponent<EffectRenderer>()->PlayEffect();

	if (effectObjects.size() > 1 && INPUT.GetKeyDown('2'))
		effectObjects[1]->GetComponent<EffectRenderer>()->PlayEffect();

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

	// SwordThunder(8번 이펙트)는 매 프레임 플레이어 손 본을 추적해 위치/회전 갱신
	if (effectObjects.size() > 8 && effectObjects[8] && myPlayer) {
		if (auto effectRenderer = effectObjects[8]->GetComponent<EffectRenderer>())
		{
			auto animator = myPlayer->GetComponent<Animator>();
			auto transform = myPlayer->GetComponent<Transform>();

			if (animator && animator->IsInitialized())
			{
				XMFLOAT3 bonePos = animator->GetBonePosition(45);
				XMVECTOR boneRot = animator->GetBoneRotation(45);

				XMMATRIX worldMat = transform->GetWorldMatrix();
				XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

				// z축 90도 초기 회전 - DirectX12와 effekseer 축 차이
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

	if (myPlayer)
	{
		auto& pos = myPlayer->GetComponent<Transform>()->GetPosition();
		if (pos.x < 502.0f)
		{
		    auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Plaza);
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

	/*auto start = chrono::high_resolution_clock::now();*/
	for (auto& batch : instancingBatches)
		batch->Update(frustum, camPosVec);
	//auto end = chrono::high_resolution_clock::now();
	//auto ms = chrono::duration_cast<chrono::microseconds>(end - start).count();
	//OutputDebugStringA(("Update: " + to_string(ms) + "us\n").c_str());
}

void PlazaScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());
	//renderer->RenderCollisionMeshWireframe(*coreRef, gameObjects);

	// Render terrain
	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

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

void PlazaScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());
}

void PlazaScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
}

void PlazaScene::RenderSceneEffects()
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

void PlazaScene::RequestSceneChange()
{
	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestLoadingScene(SceneType::Village);
	}
}

void PlazaScene::CreateKnightPool()
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

void PlazaScene::CreateBossObject()
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

void PlazaScene::CreateImpObject()
{
	impObject = make_shared<GameObject>();
	impObject->SetId(-1);
	auto mesh = impObject->AddComponent<Mesh>();
	auto transform = impObject->AddComponent<Transform>();
	auto animator = impObject->AddComponent<Animator>();
	auto animMachine = impObject->AddComponent<AnimationMachine>();
	mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/Imp/monster_Imp");

	animMachine->SetAnimationSet(AnimationSetFactory::CreateImpSet());
	transform->SetInitPosition(22.f, SampleHeightAt(22.0f, 22.0f), 22.f);
	transform->SetRotation(0.f, 3.14f, 0.f);
	transform->SetScale(0.01f, 0.01f, 0.01f);
	AddGameObject(impObject);
}

void PlazaScene::CreateEffectSamples()
{
	struct EffectInfo {
		u16string name;
		float x;
		float y;
		float z;
	};

	vector<EffectInfo> info = {
		{u"Fireworks", 484.607025f, 6.f, 481.862946f},
		{u"BloodLance", 484.607025f, 6.f, 481.862946f},
		{u"Aura01_HDR2", 484.607025f, 6.f, 481.862946f},
		{u"Benediction", 484.607025f, 10.f, 481.862946f},
		{u"Atmosphere", 484.607025f, 10.f, 481.862946f},
		{u"CandleFire5", 484.607025f, 6.f, 481.862946f},
		{u"CandleFire5", 475.607025f, 6.f, 481.862946f},
		{u"Dissolve", 484.607025f, 6.f, 481.862946f},
		{u"SwordThunder", 484.607025f, 6.f, 481.862946f}
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
		transform->SetScale(1.f, 1.f, 1.f);
		effectObjects.push_back(effectSample);
		AddGameObject(effectSample);
	}
}

float PlazaScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

void PlazaScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void PlazaScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
{
	NetId nid{ add.netid() };
	int id = nid.GetId();
	int type = add.typeid_();

	if (type == static_cast<int>(CharacterId::FinalBoss)) // Final_Boss
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
		}

		if (id == INPUT.GetClientID())
		{
			myPlayer = player;
			myPlayer->SetAsLocalPlayer(cam.get());

			IMGUI.SetMyPlayer(myPlayer.get());

			OutputDebugStringA("My character activated!\n");
		}
	}
	else if (type == static_cast<int>(CharacterId::Imp))
	{
		if (impObject)
		{
			impObject->SetId(id);
			auto transform = impObject->GetComponent<Transform>();
			transform->SetInitPosition(add.x(), add.y(), add.z());
			transform->SetTargetRotation(add.yaw());
			activeCharacters[id] = impObject;
		}
	}
}

void PlazaScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
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

void PlazaScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void PlazaScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void PlazaScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Plaza);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}