#include "pch.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "WindowInfo.h"
#include "Timer.h"
#include "Input.h"
#include "GraphicsManager.h"

void Engine::Init()
{
	network = new NetworkManager();
	network->Init("127.0.0.1", 9000);		// 동환이가 주는 IP & 포트번호 넣어야함

	GET_SINGLE(WindowInfo)->Init();
	GET_SINGLE(Timer)->Init();

	graphics = new GraphicsManager();
	graphics->Init();
	graphics->SetNetworkManager(network);

	network->SetGraphicsManager(graphics);

	input = new Input();
	input->SetCamera(graphics->GetCamera());
	input->SetNetworkManager(network);
	input->SetGraphicsManager(graphics);

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowUserPointer(window, input);
	glfwSetKeyCallback(window, Input::KeyBoardInput);
	glfwSetScrollCallback(window, Input::Scroll_callback);
	glfwSetMouseButtonCallback(window, Input::MouseFunc);
	glfwSetCursorPosCallback(window, Input::MouseMoveFunc);
}

void Engine::Update()
{
	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();

	while (!glfwWindowShouldClose(window)) {
		network->Update();
		GET_SINGLE(Timer)->Update();
		//input->Update(window);
		graphics->Update();
		graphics->Render(window);
		ShowFps();

		// TODO

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Engine::Release()
{
	graphics->Release();
	delete input;
	delete graphics;

	network->Release();
	delete network;
}

void Engine::ShowFps()
{
	uint32 fps = GET_SINGLE(Timer)->GetFps();

	char text[100];
	sprintf_s(text, "FPS : %d", fps);

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowTitle(window, text);
}
