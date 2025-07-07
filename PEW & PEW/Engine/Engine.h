#pragma once

class Camera;
class MainCharacter;
class Input;
class StaticObject;
class ShadowMapping;
class Enemy;
class GraphicsManager;

class Engine
{
public:
	void Init();
	void Update();
	void Release();

private:
	void ShowFps();

private:
	Input* input = { nullptr };
	GraphicsManager* graphics;
};
