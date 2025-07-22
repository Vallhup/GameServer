#include "pch.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "GraphicsManager.h"
#include "Input.h"
#include "WindowInfo.h"

void SceneManager::Init()
{
	currentScene = SceneType::Scene1;
	InitScene1();
}

void SceneManager::Update(GLFWwindow* window)
{
	if (currentScene == SceneType::Scene2)
		UpdateScene2();

	input->Update(window);
	graphics->Update(currentScene);
}

void SceneManager::Render(GLFWwindow* window)
{
	graphics->Render(window, currentScene);
}

void SceneManager::Release()
{
	ReleaseScene2();
}

void SceneManager::ChangeScene(SceneType newScene)
{
	if (currentScene == newScene)
		return;

	if (currentScene == SceneType::Scene1 && newScene == SceneType::Scene2)
	{
		input->SetMainCharacter(nullptr);
		currentScene = newScene;
		graphics->RemoveCharacter(0);
		InitScene2();
	}
}

void SceneManager::InitScene1()
{
	graphics = new GraphicsManager();
	graphics->Init();
	graphics->SetSceneManager(this);

	input = new Input();
	input->SetCamera(graphics->GetCamera());
	input->SetGraphicsManager(graphics);
	input->SetSceneType(currentScene);

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowUserPointer(window, input);
	glfwSetKeyCallback(window, Input::KeyBoardInput);
	glfwSetScrollCallback(window, Input::Scroll_callback);
	glfwSetCursorPosCallback(window, Input::MouseMoveFunc);
}

void SceneManager::InitScene2()
{
	network = new NetworkManager();
	network->Init("127.0.0.1", 9000);		// 동환이가 주는 IP & 포트번호 넣어야함
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

}

void SceneManager::ReleaseScene2()
{
	graphics->Release();
	delete input;
	delete graphics;

	network->Release();
	delete network;
}
