#pragma once
#include "Scene.h"

enum class SceneType {
    Start,
    Login,
    ServerSquare,
    MainGame,
    END
};

class SceneManager
{
public:
    ~SceneManager();
    void Initialize(HWND hWnd, DX12Core& core);
    void Update(const float deltaTime);
    void BeginRender();
    void RenderDeferred();
    void RenderForward();
    void RenderShadow();
    void RenderEffects();
    void Release();

    Scene* GetCurrentScene() const;
    SceneRenderer* GetSceneRenderer() const;

public:
    void SceneStart(DX12Core& core);       
    void RequestSceneChange(SceneType type);
    void ProcessPendingSceneChange(DX12Core& core);  

private:
    template <typename T>
    void RegisterScene(SceneType type);

private:
    HWND hwnd;
    Scene* mCurrentScene = nullptr;
    std::array<std::unique_ptr<Scene>, static_cast<size_t>(SceneType::END)> mScenes;

    bool pendingSceneChange = false;
    SceneType nextSceneType;

    unique_ptr<SceneRenderer> sceneRenderer;
};

template<typename T>
inline void SceneManager::RegisterScene(SceneType type)
{
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");
    size_t index = static_cast<size_t>(type);
    mScenes[index] = std::make_unique<T>();
}
