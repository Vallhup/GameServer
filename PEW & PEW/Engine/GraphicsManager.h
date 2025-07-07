#pragma once

class Camera;
class MainCharacter;
class ShadowMapping;
class Enemy;

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

private:
	Camera* camera = { nullptr };
	MainCharacter* mainCat = { nullptr };
	ShadowMapping* shadowMap = { nullptr };
	Enemy* enemy[3][9] = { nullptr };
};

