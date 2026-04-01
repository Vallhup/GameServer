#pragma once
#include "Scene.h"

enum class SceneType {
    Title,
    Select,
    Plaza,
    Village,
    Castle,
    Final,
    Loading,
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
    SceneType GetCurrentSceneType() const;
    SceneRenderer* GetSceneRenderer() const;

public:
    void SceneStart(DX12Core& core);       
    void RequestSceneChange(SceneType type);
    void RequestLoadingScene(SceneType targetSceneType);
    void ProcessPendingSceneChange(DX12Core& core);

    //----
    // 임시 코드임, First->Second 연결 해보려고 시도하는 코드임
    void SetSharedKnight(shared_ptr<MainCharacter> k) { sharedKnight = k; }
    void SetSharedBoss(shared_ptr<GameObject> b) { sharedBoss = b; }

    shared_ptr<MainCharacter> GetSharedKnight() { return sharedKnight; }
    shared_ptr<GameObject> GetSharedBoss() { return sharedBoss; }
    //----

private:
    template <typename T>
    void RegisterScene(SceneType type);

    void MoveInstancingBatches(SceneType type);

private:
    HWND hwnd;
    Scene* mCurrentScene = nullptr;
    std::array<std::unique_ptr<Scene>, static_cast<size_t>(SceneType::END)> mScenes;

    bool pendingSceneChange = false;
    SceneType currSceneType;
    SceneType nextSceneType;

    unique_ptr<SceneRenderer> sceneRenderer;

    //----
    // 임시 코드임, First->Second 연결 해보려고 시도하는 코드임
    shared_ptr<MainCharacter> sharedKnight;  // 내 캐릭터
    shared_ptr<GameObject> sharedBoss;       // 보스
    int myClientId = -1;
    //----
};

template<typename T>
inline void SceneManager::RegisterScene(SceneType type)
{
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");
    size_t index = static_cast<size_t>(type);
    mScenes[index] = std::make_unique<T>();
}
