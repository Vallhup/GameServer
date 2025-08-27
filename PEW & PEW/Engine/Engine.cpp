#include "pch.h"
#include "Engine.h"
#include "SoundManager.h"
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

	soundManager = new SoundManager();
	soundManager->Init();

	sceneManager = new SceneManager();
	sceneManager->Init(*soundManager);
}

void Engine::Update()
{
	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();

	while (!glfwWindowShouldClose(window)) {
		GET_SINGLE(Timer)->Update();
		const float deltaTime = GET_SINGLE(Timer)->GetDeltaTime();
		sceneManager->Update(window, deltaTime);
		sceneManager->Render();

		ShowFps();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Engine::Release()
{
	soundManager->Release();
	delete soundManager;

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
