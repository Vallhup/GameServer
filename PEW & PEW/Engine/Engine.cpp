#include "pch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "WindowInfo.h"
#include "Timer.h"
#include "Input.h"
#include "GraphicsManager.h"

void Engine::Init()
{
	GET_SINGLE(WindowInfo)->Init();
	GET_SINGLE(Timer)->Init();

	sceneManager = new SceneManager();
	sceneManager->Init();
}

void Engine::Update()
{
	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();

	while (!glfwWindowShouldClose(window)) {
		GET_SINGLE(Timer)->Update();
		sceneManager->Update(window);
		sceneManager->Render();

		ShowFps();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Engine::Release()
{
	sceneManager->Release();
	delete sceneManager;
}

void Engine::ShowFps()
{
	uint32 fps = GET_SINGLE(Timer)->GetFps();

	char text[100];
	sprintf_s(text, "FPS : %d", fps);

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowTitle(window, text);
}
