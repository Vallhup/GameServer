#include "pch.h"
#include "SecondBattleScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Engine.h"
#include "Terrain.h"
#include "SkyBox.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "SoundManager.h"
#include "EffectManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "AnimationMachine.h"
#include "NetId.h"
#include "NetHelper.h"

void SecondBattleScene::Release()
{
}

void SecondBattleScene::Reset()
{
	instancingBatches.clear();
	activeCharacters.clear();
	gameObjects.clear();
	myPlayer = nullptr;
	bossObject = nullptr;

	OutputDebugStringA("SecondBattleScene Data has been deleted!! \n----------------------------------------\n");
}

void SecondBattleScene::InitializeSceneObjectPools()
{
}

void SecondBattleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nSecondBattleScene Data has been created!! \n");

	// SceneManager에서 공유 캐릭터 받아오기
	myPlayer = sManagerRef->GetSharedKnight();
	bossObject = sManagerRef->GetSharedBoss();

	if (myPlayer)
	{
		myPlayer->SetAsLocalPlayer(cam.get());
		activeCharacters[myPlayer->GetId()] = myPlayer;
		gameObjects.push_back(myPlayer);

		// Castle 맵 초기 위치로 재설정
		auto transform = myPlayer->GetComponent<Transform>();
		transform->SetInitPosition(333.9609f, 67.95122f, 234.2314f);

		IMGUI.SetMyPlayer(myPlayer.get());
		OutputDebugStringA("SecondBattle: MyPlayer loaded from shared!\n");
	}

	if (bossObject)
	{
		activeCharacters[bossObject->GetId()] = bossObject;
		gameObjects.push_back(bossObject);
		OutputDebugStringA("SecondBattle: Boss loaded from shared!\n");
	}

	// SkyBox 초기화
	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList());
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());

	// Castle Terrain 초기화
#pragma region Initialize Castle Terrain
	terrain = make_shared<Terrain>();
	terrain->Initialize(*coreRef, L"textures/CastleFloor", L"../Assets/FBXModel/CastleMap/castleTerrain.raw", 513, 650.2402f, 79.28662f, 8.0f);
#pragma endregion

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	OutputDebugStringA("SecondBattleScene initialized!\n");
}

void SecondBattleScene::UpdateScene(const float deltaTime)
{
	if (myPlayer)
	{
		auto transform = myPlayer->GetComponent<Transform>();
		coreRef->SetPlayerPosForShadow(transform->GetPosition());

		if (INPUT.GetKeyDown('0'))
		{
			XMFLOAT3 pos = transform->GetPosition();
			OutputDebugStringA(("MyPlayer Pos: " + to_string(pos.x) + ", " + to_string(pos.y) + ", " + to_string(pos.z) + "\n").c_str());
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

	for (auto& batch : instancingBatches)
		batch->Update(frustum, camPosVec);
}

void SecondBattleScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());

	// Render terrain
	if (terrain)
		renderer->RenderTerrain(*coreRef, terrain.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}
}

void SecondBattleScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());

	renderer->RenderForward(*coreRef, gameObjects, cam.get());
}

void SecondBattleScene::RenderSceneShadow()
{
	auto renderer = sManagerRef->GetSceneRenderer();
	renderer->RenderShadow(*coreRef, gameObjects);

	for (const auto& batch : instancingBatches)
	{
		batch->RenderShadow(*coreRef, renderer);
	}
}

void SecondBattleScene::RenderSceneEffects()
{
	if (cam)
		EFFECT_MANAGER->Render(*coreRef, cam.get());
}

void SecondBattleScene::RequestSceneChange()
{
}

float SecondBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	if (terrain)
		return terrain->SampleHeightAt(worldX, worldZ);
	return 0.0f;
}

void SecondBattleScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void SecondBattleScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
{
	//NetId nid{ add.netid() };
	//int id = nid.GetId();
	//int type = add.typeid_();

	//if (type == 5) // Final_Boss
	//{
	//	if (bossObject)
	//	{
	//		bossObject->SetId(id);
	//		auto transform = bossObject->GetComponent<Transform>();
	//		transform->SetInitPosition(add.x(), add.y(), add.z());
	//		transform->SetTargetRotation(add.yaw());
	//		activeCharacters[id] = bossObject;
	//	}
	//}
	//else if (type == 1) // Knight
	//{
	//	auto player = GetAvailableKnight();
	//	if (player)
	//	{
	//		player->SetId(id);
	//		auto transform = player->GetComponent<Transform>();
	//		transform->SetInitPosition(add.x(), add.y(), add.z());
	//		transform->SetTargetRotation(add.yaw());
	//		activeCharacters[id] = player;
	//	}

	//	if (id == INPUT.GetClientID())
	//	{
	//		myPlayer = player;
	//		myPlayer->SetAsLocalPlayer(cam.get());

	//		IMGUI.SetMyPlayer(myPlayer.get());

	//		OutputDebugStringA("My character activated!\n");
	//	}
	//}
}

void SecondBattleScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
{
	// First -> Second 맵 오프셋 (임시)
	constexpr float offsetX = 323.0f;
	constexpr float offsetZ = 208.0f;

	NetId nid{ move.netid() };
	int id = nid.GetId();
	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		auto transform = it->second->GetComponent<Transform>();

		float worldX = move.x() + offsetX - 156.0f;	// move.x 대략 163	이거 뺀 값은 첫 씬 좌표 (서버에서 init하는)
		float worldZ = move.z() + offsetZ - 650.0f;	// move.z 대략 643
		transform->SetPosition(worldX, SampleHeightAt(worldX, worldZ), worldZ);
		transform->SetTargetRotation(move.yaw());
	}
}

void SecondBattleScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void SecondBattleScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void SecondBattleScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Castle);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}