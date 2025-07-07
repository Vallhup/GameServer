#include "pch.h"
#include "Engine.h"
#include "WindowInfo.h"
#include "Timer.h"
#include "Input.h"
#include "GraphicsManager.h"

void Engine::Init()
{
	GET_SINGLE(WindowInfo)->Init();
	GET_SINGLE(Timer)->Init();

	graphics = new GraphicsManager();
	graphics->Init();

	input = new Input();
	input->SetCamera(graphics->GetCamera());
	input->SetMainCharacter(graphics->GetMainCat());

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowUserPointer(window, input);
	glfwSetKeyCallback(window, Input::KeyBoardInput);
	glfwSetScrollCallback(window, Input::Scroll_callback);
	glfwSetMouseButtonCallback(window, Input::MouseFunc);
}

void Engine::Update()
{
	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();

	while (!glfwWindowShouldClose(window)) {
		GET_SINGLE(Timer)->Update();

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
}

void Engine::ShowFps()
{
	uint32 fps = GET_SINGLE(Timer)->GetFps();

	char text[100];
	sprintf_s(text, "FPS : %d", fps);

	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	glfwSetWindowTitle(window, text);
}
