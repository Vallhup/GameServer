#pragma once
#include "Scene.h"

class UploadBuffer;

class TestScene final : public Scene
{
public:
	TestScene() = default;
	TestScene(const TestScene&) = delete;
	TestScene& operator=(const TestScene&) = delete;
	~TestScene();

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
	shared_ptr<MainCharacter> knight;
};
