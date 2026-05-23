#include "pch.h"
#include "SelectScene.h"
#include "SceneManager.h"
#include "Input.h"
#include "MainCharacter.h"
#include "Animator.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "NetId.h"
#include "SoundManager.h"

void SelectScene::Release()
{
	SOUND_MANAGER->StopBGM(1.0f);

	gameObjects.clear();
	bigDemonWarrior.reset();

	OutputDebugStringA("SelectScene Data has been deleted!! \n----------------------------------------\n");
}

const char* SelectScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/SelectBGM.mp3";
}

float SelectScene::GetBGMFadeInSeconds() const
{
	return 0.5f;
}

void SelectScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nSelectScene Data has been created!! \n");

	{
		bigDemonWarrior = make_shared<MainCharacter>();
		auto mesh = bigDemonWarrior->AddComponent<Mesh>();
		mesh->SetTwoSided(true);
		auto transform = bigDemonWarrior->AddComponent<Transform>();
		auto animator = bigDemonWarrior->AddComponent<Animator>();
		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/BigDemonWarrior/monster_BigDemonWarrior");
		transform->SetInitPosition(0.f, 0.f, 0.f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		gameObjects.push_back(bigDemonWarrior);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		mesh->ReleaseUploadBuffers();

		bigDemonWarrior->SetAsLocalPlayer(cam.get());
	}

	{
		tank = make_shared<GameObject>();
		auto mesh = tank->AddComponent<Mesh>();
		mesh->SetTwoSided(true);
		auto transform = tank->AddComponent<Transform>();
		auto animator = tank->AddComponent<Animator>();

		mesh->SetMesh(*coreRef, L"../Assets/FBXModel/Monster/Tank/monster_Tank");
		transform->SetInitPosition(5.f, 0.f, 0.5f);
		transform->SetRotation(0.f, 0.f, 0.f);
		transform->SetScale(0.01f, 0.01f, 0.01f);
		gameObjects.push_back(tank);

		coreRef->FlushCommandQueue();
		coreRef->ResetCommandQueue();

		mesh->ReleaseUploadBuffers();

		OutputDebugStringA("Dragon created!!\n");
	}
}

void SelectScene::InitializeSceneMonsters()
{
}

void SelectScene::UpdateScene(const float deltaTime)
{
	auto animator = bigDemonWarrior->GetComponent<Animator>();
	auto animator2 = tank->GetComponent<Animator>();
	if (animator && animator2) {
		if (INPUT.GetKeyDown('1')) 
		{
			animator->TransitionToAnimation(0, 0.4f);
			animator2->TransitionToAnimation(0, 0.4f);
		}
		if (INPUT.GetKeyDown('2')) 
		{
			animator->TransitionToAnimation(1, 0.4f);
			animator2->TransitionToAnimation(1, 0.4f);
		}
		if (INPUT.GetKeyDown('3')) 
		{
			animator->TransitionToAnimation(2, 0.4f);
			animator2->TransitionToAnimation(2, 0.4f);
		}
		if (INPUT.GetKeyDown('4')) 
		{
			animator->TransitionToAnimation(3, 0.4f);
			animator2->TransitionToAnimation(3, 0.4f);
		}
		if (INPUT.GetKeyDown('5'))
		{
			animator->TransitionToAnimation(4, 0.4f);
			animator2->TransitionToAnimation(4, 0.4f);
		}
		if (INPUT.GetKeyDown('6'))
		{
			animator->TransitionToAnimation(5, 0.4f);
			animator2->TransitionToAnimation(5, 0.4f);
		}
		if (INPUT.GetKeyDown('7'))
		{
			animator->TransitionToAnimation(6, 0.4f);
			animator2->TransitionToAnimation(6, 0.4f);
		}
		if (INPUT.GetKeyDown('8'))
		{
			animator->TransitionToAnimation(7, 0.4f);
			animator2->TransitionToAnimation(7, 0.4f);
		}
		if (INPUT.GetKeyDown('9'))
		{
			animator->TransitionToAnimation(8, 0.4f);
			animator2->TransitionToAnimation(8, 0.4f);
		}
		if (INPUT.GetKeyDown('0'))
		{
			animator->TransitionToAnimation(9, 0.4f);
			animator2->TransitionToAnimation(9, 0.4f);
		}
		if (INPUT.GetKeyDown('Q'))
		{
			animator->TransitionToAnimation(10, 0.4f);
			animator2->TransitionToAnimation(10, 0.4f);
		}
		if (INPUT.GetKeyDown('W'))
		{
			animator->TransitionToAnimation(11, 0.4f);
			animator2->TransitionToAnimation(11, 0.4f);
		}
		if (INPUT.GetKeyDown('E'))
		{
			animator->TransitionToAnimation(12, 0.4f);
			animator2->TransitionToAnimation(12, 0.4f);
		}
		if (INPUT.GetKeyDown('R'))
		{
			animator->TransitionToAnimation(13, 0.4f);
			animator2->TransitionToAnimation(13, 0.4f);
		}
		if (INPUT.GetKeyDown('T'))
		{
			animator->TransitionToAnimation(14, 0.4f);
			animator2->TransitionToAnimation(14, 0.4f);
		}
		if (INPUT.GetKeyDown('Y'))
		{
			animator->TransitionToAnimation(15, 0.4f);
			animator2->TransitionToAnimation(15, 0.4f);
		}
		if (INPUT.GetKeyDown('U'))
		{
			animator->TransitionToAnimation(16, 0.4f);
			animator2->TransitionToAnimation(16, 0.4f);
		}
		if (INPUT.GetKeyDown('I'))
		{
			animator->TransitionToAnimation(17, 0.4f);
			animator2->TransitionToAnimation(17, 0.4f);
		}
		if (INPUT.GetKeyDown('O'))
		{
			animator->TransitionToAnimation(18, 0.4f);
			animator2->TransitionToAnimation(18, 0.4f);
		}
		if (INPUT.GetKeyDown('P'))
		{
			animator->TransitionToAnimation(19, 0.4f);
			animator2->TransitionToAnimation(19, 0.4f);
		}
		if (INPUT.GetKeyDown('A'))
		{
			animator->TransitionToAnimation(20, 0.4f);
			animator2->TransitionToAnimation(20, 0.4f);
		}
		if (INPUT.GetKeyDown('S'))
		{
			animator->TransitionToAnimation(21, 0.4f);
			animator2->TransitionToAnimation(21, 0.4f);
		}
		if (INPUT.GetKeyDown('D'))
		{
			animator->TransitionToAnimation(22, 0.4f);
			animator2->TransitionToAnimation(22, 0.4f);
		}
		if (INPUT.GetKeyDown('F'))
		{
			animator->TransitionToAnimation(23, 0.4f);
			animator2->TransitionToAnimation(23, 0.4f);
		}
		if (INPUT.GetKeyDown('G'))
		{
			animator->TransitionToAnimation(24, 0.4f);
			animator2->TransitionToAnimation(24, 0.4f);
		}
		if (INPUT.GetKeyDown('H'))
		{
			animator->TransitionToAnimation(25, 0.4f);
			animator2->TransitionToAnimation(25, 0.4f);
		}
		if (INPUT.GetKeyDown('J'))
		{
			animator->TransitionToAnimation(26, 0.4f);
			animator2->TransitionToAnimation(26, 0.4f);
		}
		if (INPUT.GetKeyDown('K'))
		{
			animator->TransitionToAnimation(27, 0.4f);
			animator2->TransitionToAnimation(27, 0.4f);
		}
		if (INPUT.GetKeyDown('L'))
		{
			animator->TransitionToAnimation(28, 0.4f);
			animator2->TransitionToAnimation(28, 0.4f);
		}
		if (INPUT.GetKeyDown('Z'))
		{
			animator->TransitionToAnimation(29, 0.4f);
			animator2->TransitionToAnimation(29, 0.4f);
		}
		if (INPUT.GetKeyDown('X'))
		{
			animator->TransitionToAnimation(30, 0.4f);
			animator2->TransitionToAnimation(30, 0.4f);
		}
		if (INPUT.GetKeyDown('C'))
		{
			animator->TransitionToAnimation(31, 0.4f);
			animator2->TransitionToAnimation(31, 0.4f);
		}
		if (INPUT.GetKeyDown('V'))
		{
			animator->TransitionToAnimation(32, 0.4f);
			animator2->TransitionToAnimation(32, 0.4f);
		}
		if (INPUT.GetKeyDown('B'))
		{
			animator->TransitionToAnimation(33, 0.4f);
			animator2->TransitionToAnimation(33, 0.4f);
		}
		if (INPUT.GetKeyDown('N'))
		{
			animator->TransitionToAnimation(34, 0.4f);
			animator2->TransitionToAnimation(34, 0.4f);
		}
	}
	

	for (const auto& obj : gameObjects)
		obj->Update(deltaTime);

	if (cam)
		cam->Update(*coreRef, deltaTime, gameObjects, {}, bigDemonWarrior);
}

void SelectScene::RenderSceneDeferred()
{
	sManagerRef->GetSceneRenderer()->RenderDeferred(*coreRef, gameObjects, cam.get());
}

