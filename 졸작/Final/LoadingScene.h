#pragma once
#include "Scene.h"

enum class SceneType;

class LoadingScene final : public Scene
{
public:
	LoadingScene() = default;
	LoadingScene(const LoadingScene&) = delete;
	LoadingScene& operator=(const LoadingScene&) = delete;
	~LoadingScene();

	void Release() override;

	vector<shared_ptr<InstancingBatch>> TakeBatches(SceneType type);
	void SetTargetScene(SceneType type) { targetScene = type; }

protected:
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;

private:
	void LoadPlazaSceneResources();
	void LoadFirstBattleSceneResources();
	void LoadSecondBattleSceneResources();
	void LoadFinalBattleSceneResources();

private:
	SceneType targetScene;
	unordered_map<SceneType, vector<shared_ptr<InstancingBatch>>> sceneBatches;

	queue<function<void()>> loadTasks;
	int totalTasks = 0;
	int completedTasks = 0;
};

