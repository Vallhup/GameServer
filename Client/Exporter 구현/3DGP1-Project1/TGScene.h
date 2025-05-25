#pragma once
#include "Scene.h"
#include "Enemy.h"
#include "Bullet.h"

class TankGame final : public Scene
{
public:
	TankGame() = default;
	TankGame(const TankGame&) = delete;
	TankGame& operator=(const TankGame&) = delete;
	~TankGame() = default;

	void Release() override;
	void Reset() override;

	template <typename T>
	void DeleteEach(T*& member);

protected:
	const float* GetBackgroundColor() override;
	void InitializeProjection() override;
	void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
	void UpdateLogic(const float deltaTime) override;
	const GameObject* GetWorld() const override;
	int GetSceneWidth() const override;

private:
	void InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void InitializeGround();
	void InitializeTank();
	void InitializeEnemies(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, GameObject* world);
	void InitializeBullets(GameObject* world, VertexIndexBuffer* cube);
	void InitializeObstacles();
	void InitializeVicLett();

	void SetEnemyStartPosition();

	void MoveTank(const float deltaTime);
	void GetMousePosAndPicking();
	void EndingKeyLogic();

	void UpdateEnemies(float deltaTime);
	void UpdateTankBodyRotation(float deltaTime);
	void UpdateTurretRotation(float deltaTime);
	void UpdateBullets(float deltaTime);
	void UpdateEnemyBullets();
	void ProcessTankFire();
	void ToggleShield();

	void ShowVictoryText();

	void OnTankHit();

	bool CheckVictory();

	BoundingBox GetBoundingBox() const;

private:
	unique_ptr<VertexIndexBuffer> tPlane = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tCube = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tObstacle = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tBody = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tHead = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tBarrel = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tBullet = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> tLetter[7] = {};

	GameObject tWorld = {};
	GameObject tGround = {};
	GameObject tTankBody = {};
	GameObject tTankHead = {};
	GameObject tTankBarrel = {};
	GameObject tTankFiringPoint = {};
	GameObject tTankShield = {};
	GameObject tMousePoint = {};

	GameObject tVictoryLetter[7] = {};

	array<Bullet, 10> tBullets = {};
	int tBulletCount = 0;

	array<Enemy, 20> tEnemies = {};

	array<GameObject, 10> tObstacles = {};

	Enemy* pickedEnemy = nullptr;

	BoundingBox tTankAABB = {};

	int tTankLife = 10;

	const float color[4] = { 0.58823f, 0.58823f, 0.58823f, 1.0f };

	static constexpr XMFLOAT3 TGSCENE_OFFSET = { 0.9f, 1.8f, -0.9f };
	static constexpr int TANK_GAME_WIDTH = 2;
};
