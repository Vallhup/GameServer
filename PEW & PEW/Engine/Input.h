#pragma once
#include "SceneManager.h"

class Camera;
class MainCharacter;
class NetworkManager;
class GraphicsManager;

class Input
{
public:
	static void KeyBoardInput(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void MouseMoveFunc(GLFWwindow* window, double xpos, double ypos);

	void Update(GLFWwindow* window);

	void CheckContinuousAttack(GLFWwindow* window);
	char GetCurrentDirection();
	void SendMovePacket();
	void SendAttackPacket();
	void SendAttackEndPacket();

	void SetCamera(Camera* cam) { camera = cam; }
	void SetMainCharacter(MainCharacter* cat) { mainCat = cat; }
	void SetNetworkManager(NetworkManager* net) { network = net; }
	void SetGraphicsManager(GraphicsManager* gfx) { graphics = gfx; }
	void SetSceneType(SceneType type) { sceneType = type; }

private:
	Camera* camera = { nullptr };
	MainCharacter* mainCat = { nullptr };
	NetworkManager* network = { nullptr };
	GraphicsManager* graphics = { nullptr };

	double lastMouseAngle = { 0.0f };
	bool isAttacking = { false };           // 현재 공격 중인지
	bool wasFireAnimation = { false };      // 이전 프레임이 공격 애니메이션이었는지
	SceneType sceneType;
};
