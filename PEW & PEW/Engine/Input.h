#pragma once

class Camera;
class MainCharacter;

class Input
{
public:
	static void KeyBoardInput(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void MouseFunc(GLFWwindow* window, int button, int action, int mods);

	void SetCamera(Camera* cam) { camera = cam; }
	void SetMainCharacter(MainCharacter* cat) { mainCat = cat; }

private:
	Camera* camera = { nullptr };
	MainCharacter* mainCat = { nullptr };
};
