#pragma once
#include "Scene.h"

class GameObject;

class StartScene final : public Scene
{
public:
	StartScene() = default;
	StartScene(const StartScene&) = delete;
	StartScene& operator=(const StartScene&) = delete;
	~StartScene();

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
	void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
	void UpdateLogic(const float deltaTime) override;
	void RenderScene() override;
	int GetSceneWidth() const override;

private:
	array<shared_ptr<GameObject>, 100> knights;
	shared_ptr<GameObject> knight;
};

