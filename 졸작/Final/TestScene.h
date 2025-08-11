#pragma once
#include "Scene.h"

class GameObject;

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
	void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
	void UpdateScene(const float deltaTime) override;
	void RenderScene() override;
	int GetSceneWidth() const override;

private:
	array<shared_ptr<GameObject>, 100> knights;
	shared_ptr<GameObject> knight;
};

