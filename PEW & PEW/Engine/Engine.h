#pragma once

class NetworkManager;
class GraphicsManager;
class Input;

class Engine
{
public:
	void Init();
	void Update();
	void Release();

private:
	void ShowFps();

private:
	NetworkManager* network;
	GraphicsManager* graphics;
	Input* input = { nullptr };
};
