#pragma once
#include "Scene.h"

enum class SceneType {
    Start,
    Menu,
    Scene1,
    Scene2,
    END
};

class SceneManager
{
public:
    static SceneManager& Get();

    ~SceneManager();

    void Initialize(HWND hwnd);
    void Update(const float deltaTime);
    void Render();
    void Release();

    template <typename T>
    void RegisterScene(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, SceneType type);

    Scene* GetCurrentScene() const;

public:
    void SceneStart();
    void ChangeScene(SceneType type);

private:
    HWND mHwnd = nullptr;
    Scene* mCurrentScene = nullptr;
    std::array<std::unique_ptr<Scene>, static_cast<size_t>(SceneType::END)> mScenes;
};

template<typename T>
inline void SceneManager::RegisterScene(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, SceneType type)
{
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");
    size_t index = static_cast<size_t>(type);
    mScenes[index] = std::make_unique<T>();
    mScenes[index]->Initialize(device, cmdList);
}
