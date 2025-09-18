#pragma once

enum class SceneType {
    Scene1,
    Scene2
};

class SoundManager;
class NetworkManager;
class GraphicsManager;
class Input;

class SceneManager
{
public:
    void Init(SoundManager& soundmanager);
    void Update(GLFWwindow* window, const float deltaTime);
    void Render();
    void Release();

    void TransitionUpdate(const float deltaTime);
    void ChangeScene(SceneType newScene);

private:
    void InitScene1();
    void InitScene2(int characterType);

    void UpdateScene1();
    void UpdateScene2();

    void ReleaseScene1();
    void ReleaseScene2();

    void SendLoginPacket(int characterType);
    
    void SetPlayerState(PlayerPVPState state);

private:
    SoundManager* soundRef;
    NetworkManager* network;
    GraphicsManager* graphics;
    Input* input = { nullptr };

    SceneType currentScene;

    bool isTransitioning = { false };
    bool isSceneLoaded = { true };
    float loadingTimer = 3.0f;

    PlayerPVPState currentPlayerState = PlayerPVPState::WAITING;

    float readyToFightTimer = 0.0f;
    const float READY_TO_FIGHT_DELAY = 1.0f; 
    bool waitingForFightTransition = false;
};

