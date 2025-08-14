#pragma once
#include "Scene.h"

class GameObject;
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
	void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
	void UpdateScene(const float deltaTime) override;
	void RenderScene() override;
	int GetSceneWidth() const override;

private:
	shared_ptr<GameObject> knightTemplate;
	vector<XMMATRIX> knightMatrix;
	unique_ptr<UploadBuffer> instanceBuffer;

	shared_ptr<GameObject> strut;

	static constexpr int INSTANCE_COUNT = 10000;
};

