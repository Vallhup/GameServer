#pragma once
#include "Scene.h"

enum class SceneType;

class MenuScene final : public Scene
{
public:
	MenuScene() = default;
	MenuScene(const MenuScene&) = delete;
	MenuScene& operator=(const MenuScene&) = delete;
	~MenuScene() = default;

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
	void InitializeProjection() override;
	void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
	void UpdateLogic(const float deltaTime) override;
	const GameObject* GetWorld() const override;
	int GetSceneWidth() const override;

private:
	void InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void SetupObjects(GameObject* arr, int count, unique_ptr<VertexIndexBuffer>* meshes, const XMFLOAT3& startPos, const XMFLOAT3& scale, float stepX);
	void RotateObjects(GameObject* arr, int count, float deltaTime);
	void ProcessSelection(GameObject* arr, int count, SceneType type, float worldX, float worldZ);

private:
	unique_ptr<VertexIndexBuffer> M = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> Menu[4] = {};
	unique_ptr<VertexIndexBuffer> Tutorial[8] = {};
	unique_ptr<VertexIndexBuffer> Start[5] = {};
	unique_ptr<VertexIndexBuffer> Scene1[6] = {};
	unique_ptr<VertexIndexBuffer> Scene2[6] = {};
	unique_ptr<VertexIndexBuffer> End[3] = {};

	GameObject mWorld = {};
	GameObject mMenu[4] = {};
	GameObject mTutorial[8] = {};
	GameObject mStart[5] = {};
	GameObject mScene1[6] = {};
	GameObject mScene2[6] = {};
	GameObject mEnd[3] = {};
	GameObject mCenter = {};

	const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	static constexpr XMFLOAT3 MNSCENE_OFFSET = { 0.0f, 0.0f, 0.0f };
	static constexpr int MENU_WIDTH = 1;
};

