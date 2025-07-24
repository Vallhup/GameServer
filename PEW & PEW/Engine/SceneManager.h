#pragma once

enum class SceneType {
    Scene1,
    Scene2
};

class NetworkManager;
class GraphicsManager;
class Input;

class SceneManager
{
public:
    void Init();
    void Update(GLFWwindow* window);
    void Render();
    void Release();

    void TransitionUpdate();
    void ChangeScene(SceneType newScene);

private:
    void InitScene1();
    void InitScene2(int characterType);

    void UpdateScene1();
    void UpdateScene2();

    void ReleaseScene1();
    void ReleaseScene2();

    void SendLoginPacket(int characterType);

private:
    NetworkManager* network;
    GraphicsManager* graphics;
    Input* input = { nullptr };

    SceneType currentScene;

    bool isTransitioning = { false };
};

