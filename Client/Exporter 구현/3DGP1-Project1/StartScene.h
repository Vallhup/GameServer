#pragma once
#include "Scene.h"

class StartScene final : public Scene
{
public:
	StartScene() = default;
	StartScene(const StartScene&) = delete;
	StartScene& operator=(const StartScene&) = delete;
	~StartScene() = default;

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
	void InitializeLetters();
	void SetupObject(GameObject& obj, const XMFLOAT3& pos, const XMFLOAT3& scale, VertexIndexBuffer* mesh);

	void UpdateTitleRotation(float deltaTime);
	void UpdateNameRotation(float deltaTime);
	void UpdateExplosion(float deltaTime);
	bool HandleMouseOverAndClick();

	void GenerateExplosion(const std::vector<XMFLOAT3>& origins);
	XMFLOAT3 RandomDirection();

private:
	unique_ptr<VertexIndexBuffer> Name[3] = {};
	unique_ptr<VertexIndexBuffer> Title[10] = {};
	unique_ptr<VertexIndexBuffer> Cube = {};
	unique_ptr<VertexIndexBuffer> ColoredCube = {};

	GameObject sWorld = {};
	GameObject sTitle[10] = {};
	GameObject sName[3] = {};
	GameObject sCenter = {};

	bool bExploding = false;
	float explosionTime = 0.0f;
	std::vector<std::unique_ptr<GameObject>> sExplosions;
	std::vector<XMFLOAT3> explosionDirections;

	const float color[4] = { 0.7960f, 0.6431f, 0.9215f };

	static constexpr XMFLOAT3 STSCENE_OFFSET = { 0.0f, 0.0f, 0.0f };
	static constexpr int START_WIDTH = 2;
};
