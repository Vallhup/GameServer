#pragma once
#include "Scene.h"

enum class SceneType {
    Start,
    Login,
    ServerSquare,
    MainGame,
    Scene1,
    Scene2,
    END
};

class SceneManager
{
public:
    ~SceneManager();
    void Initialize(DX12Core& core);        
    void Update(const float deltaTime);
    void RenderDeferred();
    void RenderForward();
    void RenderShadow();
    void RenderEffects();
    void Release();

    Scene* GetCurrentScene() const;

public:
    void SceneStart(DX12Core& core);       
    void RequestSceneChange(SceneType type);
    void ProcessPendingSceneChange(DX12Core& core);  

private:
    template <typename T>
    void RegisterScene(SceneType type);

private:
    Scene* mCurrentScene = nullptr;
    std::array<std::unique_ptr<Scene>, static_cast<size_t>(SceneType::END)> mScenes;

    bool pendingSceneChange = false;
    SceneType nextSceneType;
};

template<typename T>
inline void SceneManager::RegisterScene(SceneType type)
{
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");
    size_t index = static_cast<size_t>(type);
    mScenes[index] = std::make_unique<T>();
}
