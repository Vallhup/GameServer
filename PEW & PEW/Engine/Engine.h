#pragma once

class SceneManager;

class Engine
{
public:
	void Init();
	void Update();
	void Release();

private:
	void ShowFps();

private:
	SceneManager* sceneManager;
};
