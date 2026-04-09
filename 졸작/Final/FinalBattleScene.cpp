#include "pch.h"
#include "FinalBattleScene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Engine.h"
#include "SkyBox.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "UIManager.h"
#include "GameSceneUIController.h"
#include "AnimationMachine.h"
#include "EffectManager.h"
#include "EffectComponent.h"
#include "NetId.h"
#include "NetHelper.h"

void FinalBattleScene::Release()
{
}

void FinalBattleScene::Reset()
{
	instancingBatches.clear();
	activeCharacters.clear();
	gameObjects.clear();
	myPlayer = nullptr;
	bossObject = nullptr;

	OutputDebugStringA("FinalBattleScene Data has been deleted!! \n----------------------------------------\n");
}

void FinalBattleScene::InitializeSceneObjectPools()
{
}

void FinalBattleScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nFinalBattleScene Data has been created!! \n");

	// SceneManager에서 공유 캐릭터 받아오기
	myPlayer = sManagerRef->GetSharedKnight();
	bossObject = sManagerRef->GetSharedBoss();

	if (myPlayer)
	{
		myPlayer->SetAsLocalPlayer(cam.get());
		activeCharacters[myPlayer->GetId()] = myPlayer;
		gameObjects.push_back(myPlayer);

		IMGUI.SetMyPlayer(myPlayer.get());
		OutputDebugStringA("FinalBattle: MyPlayer loaded from shared!\n");
	}

	if (bossObject)
	{
		activeCharacters[bossObject->GetId()] = bossObject;
		gameObjects.push_back(bossObject);
		OutputDebugStringA("FinalBattle: Boss loaded from shared!\n");
	}

	skyBox = make_shared<SkyBox>();
	skyBox->Initialize(coreRef->GetDevice(), coreRef->GetGraphicsCmdList());
	IMGUI.SetSkyBox(skyBox.get());
	IMGUI.SetCamera(GetCamera());

	coreRef->FlushCommandQueue();
	coreRef->ResetCommandQueue();

	OutputDebugStringA("FinalBattleScene initialized!\n");
}

void FinalBattleScene::UpdateScene(const float deltaTime)
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

void FinalBattleScene::RenderSceneDeferred()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	renderer->RenderDeferred(*coreRef, gameObjects, cam.get());

	for (const auto& batch : instancingBatches)
	{
		batch->Render(*coreRef, renderer);
	}
}

void FinalBattleScene::RenderSceneForward()
{
	auto renderer = sManagerRef->GetSceneRenderer();

	if (skyBox)
		skyBox->RenderSkyBox(*coreRef, coreRef->GetGraphicsCmdList());
}

void FinalBattleScene::RenderSceneShadow()
{
}

void FinalBattleScene::RenderSceneEffects()
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

void FinalBattleScene::RequestSceneChange()
{
}

float FinalBattleScene::SampleHeightAt(float worldX, float worldZ) const
{
	return 0.0f;
}

void FinalBattleScene::HandleLogin(const Protocol::SC_LOGIN_PACKET& login)
{
	NetId nid{ login.netid() };
	int id = nid.GetId();
	INPUT.SetClientID(id);
	OutputDebugStringA(("My Session ID: " + to_string(INPUT.GetClientID()) + "\n").c_str());
}

void FinalBattleScene::HandleAdd(const Protocol::SC_ADD_PACKET& add)
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

void FinalBattleScene::HandleMove(const Protocol::SC_MOVE_PACKET& move)
{
	// 서버 -> Final 맵 오프셋 (임시)
	constexpr float offsetX = -506.0f;
	constexpr float offsetZ = -481.0f;

	NetId nid{ move.netid() };
	int id = nid.GetId();
	auto it = activeCharacters.find(id);
	if (it != activeCharacters.end())
	{
		auto transform = it->second->GetComponent<Transform>();

		float worldX = move.x() + offsetX;		// 임시 예측 좌표임 (맵 기반)
		float worldZ = move.z() + offsetZ;	
		transform->SetPosition(worldX, 4.0f, worldZ);
		transform->SetTargetRotation(move.yaw());
	}
}

void FinalBattleScene::HandleRemove(const Protocol::SC_REMOVE_PACKET& remove)
{
	OutputDebugStringA("SC_REMOVE packet received\n");
}

void FinalBattleScene::HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim)
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

void FinalBattleScene::HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat)
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

	auto controller = ENGINE.GetUIManager()->GetController<GameSceneUIController>(SceneType::Final);
	if (controller)
	{
		controller->HandleStatBarChange(curHp, maxHp, curStamina, maxStamina);
		if (controller->IsStatWindowOn())
			controller->HandleStatImageChange(
				curHp, maxHp, curStamina, maxStamina,
				power, aSpeed, defense, mSpeed);
	}
}