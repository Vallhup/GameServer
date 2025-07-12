#pragma once

class Camera;
class MainCharacter;
class ShadowMapping;
class Enemy;
class NetworkManager;

class GraphicsManager
{
public:
	void Init();
	void Update();
	void Render(GLFWwindow* window);
	void RenderShadow();
	void Release();

	Camera* GetCamera() const;
	MainCharacter* GetMainCat() const;
	void SetNetworkManager(NetworkManager* net);

private:
	Camera* camera = { nullptr };
	MainCharacter* mainCat = { nullptr };
	ShadowMapping* shadowMap = { nullptr };
	Enemy* enemy[3][9] = { nullptr };
	NetworkManager* network = { nullptr };
};
