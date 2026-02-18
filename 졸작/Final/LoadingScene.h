#pragma once
#include "Scene.h"

// 매 프레임
// 선 로딩
// 후 렌더

// 어떤 씬에서 어떤 씬으로 전환 하는 동안 어떤 리소스를 로딩 할건지는 어디에 정의할 것인지?

class LoadingScene final : public Scene
{
public:
	LoadingScene() = default;
	LoadingScene(const LoadingScene&) = delete;
	LoadingScene& operator=(const LoadingScene&) = delete;
	~LoadingScene();

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
	void InitializeSceneObjectPools() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;

private:
	vector<shared_ptr<InstancingBatch>> instancingBatches;

	queue<function<void()>> loadTasks;
	int totalTasks = 0;
	int completedTasks = 0;
};

