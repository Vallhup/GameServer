#pragma once

class SoundManager;
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
	SoundManager* soundManager;
	SceneManager* sceneManager;
};
