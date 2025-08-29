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
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:
	shared_ptr<GameObject> knightTemplate;
	vector<XMMATRIX> knightMatrix;
	unique_ptr<UploadBuffer> instanceBuffer;

	shared_ptr<MainCharacter> knight;

	static constexpr int INSTANCE_COUNT = 10000;
};
