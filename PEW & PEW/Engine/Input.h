#pragma once

class Camera;
class Character;
class NetworkManager;
class GraphicsManager;

class Input
{
public:
	static void KeyBoardInput(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void MouseFunc(GLFWwindow* window, int button, int action, int mods);

	char GetCurrentDirection();
	void SendMovePacket();

	void SetCamera(Camera* cam) { camera = cam; }
	void SetMainCharacter(Character* cat) { mainCat = cat; }
	void SetNetworkManager(NetworkManager* net) { network = net; }
	void SetGraphicsManager(GraphicsManager* gfx) { graphics = gfx; }

private:
	Camera* camera = { nullptr };
	Character* mainCat = { nullptr };
	NetworkManager* network = { nullptr };
	GraphicsManager* graphics = { nullptr };
};
