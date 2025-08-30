#include "pch.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "NetworkManager.h"
#include "GraphicsManager.h"
#include "Input.h"
#include "WindowInfo.h"
#include "CollisionManager.h"
#include "Fade.h"
#include "Skybox.h"
#include "PacketFactory.h"

void SceneManager::Init(SoundManager& soundmanager)
{
	soundRef = &soundmanager;

	currentScene = SceneType::Scene1;
	InitScene1();
}

void SceneManager::Update(GLFWwindow* window, const float deltaTime)
{
	TransitionUpdate(deltaTime);

	if (currentScene == SceneType::Scene2)
		UpdateScene2();

	input->Update(window);
	graphics->Update(currentScene, *soundRef, deltaTime);
}

void SceneManager::Render()
{
	graphics->Render(currentScene, *soundRef);
}

void SceneManager::Release()
{
	ReleaseScene2();
}

void SceneManager::TransitionUpdate(const float deltaTime)
{
	if (isTransitioning) {
		Fade* fade = graphics->GetFade();
		fade->AddFadeAlpha(deltaTime);

		if (fade->GetFadeAlpha() >= 1.0f) {
			input->SetMainCharacter(nullptr);
			currentScene = SceneType::Scene2;
			int localCharType = graphics->GetCharacterType();
			graphics->RemoveCharacter(0);
			ReleaseScene1();
			InitScene2(localCharType);
			isTransitioning = false;
			isSceneLoaded = false;
			soundRef->ChangeBGM("music/wassobaesso.mp3", true);
		}

		if (!input->GetInputBlock())
			input->SetInputBlock(true);
	}
	else if (!isSceneLoaded)
	{
		loadingTimer -= deltaTime;

		if (loadingTimer <= 0.0f)
		{
			isSceneLoaded = true;
			soundRef->PlayBGM();
		}
	}
	else
	{
		Fade* fade = graphics->GetFade();

		if (fade->GetFadeAlpha() > 0.0f)
			fade->SubtractFadeAlpha(deltaTime);

		if (fade->GetFadeAlpha() <= 0.0f)
		{
			if (input->GetInputBlock() && network->CanStart() && startTimer <= 0.0f)
			{
				cout << "GameStart!!" << '\n';
				input->SetInputBlock(false);
			}
			
		}

		if (network)
		{
			if (network->CanStart() && startTimer > 0.0f)
			{
				startTimer -= deltaTime;
				cout << startTimer << '\n';
			}
		}
	}
}

void SceneManager::ChangeScene(SceneType newScene)
{
	if (currentScene == newScene)
		return;

	isTransitioning = true;
}

void SceneManager::InitScene1()
{
	graphics = new GraphicsManager();
	graphics->Init();
	graphics->SetSceneManager(this);

	GET_SINGLE(CollisionManager)->Init();

	input = new Input();
	input->SetCamera(graphics->GetCamera());
	input->SetGraphicsManager(graphics);
	input->SetSceneType(currentScene);
	input->SetSoundManager(soundRef);

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowUserPointer(window, input);
	glfwSetKeyCallback(window, Input::KeyBoardInput);
	glfwSetScrollCallback(window, Input::Scroll_callback);
	glfwSetCursorPosCallback(window, Input::MouseMoveFunc);
}

void SceneManager::InitScene2(int characterType)
{
	GET_SINGLE(Skybox)->ChangeCubeMapTexture();
	graphics->InitPVPMap();

	network = new NetworkManager();
	network->Init("127.0.0.1", 9000);		// 동환이가 주는 IP & 포트번호 넣어야함
	SendLoginPacket(characterType);
	network->SetGraphicsManager(graphics);

	input->SetNetworkManager(network);
	input->SetSceneType(currentScene);
}

void SceneManager::UpdateScene1()
{
	
}

void SceneManager::UpdateScene2()
{
	network->Update();
}

void SceneManager::ReleaseScene1()
{
	graphics->ReleaseScene1();
}

void SceneManager::ReleaseScene2()
{
	graphics->Release();
	delete input;
	delete graphics;

	network->Release();
	delete network;
}

void SceneManager::SendLoginPacket(int characterType)
{
	if (!network) return;

	vector<char> packet = PacketFactory::CSLoginPacket(characterType);
	network->Send(packet);
}
