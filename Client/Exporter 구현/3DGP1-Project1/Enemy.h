#pragma once
#include "GameObject.h"
#include "Bullet.h"

class Mesh;
class VertexIndexBuffer;

class Enemy
{
public:
	enum class enemyState {
		Reload,
		Dying,
		None
	};

public:
	Enemy() = default;
	Enemy(const Enemy&) = delete;
	Enemy& operator=(const Enemy&) = delete;
	~Enemy() = default;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, GameObject* world, const GameObject* targetObject);
	void Update(GameObject* world, float deltaTime);
	void Release(GameObject* world);
	void Fire();

	void SetPosition(const XMFLOAT3& pos);
	void SetState(enemyState in);

	enemyState GetState();
	BoundingBox GetBoundingBox() const;
	void GenerateExplosion(GameObject* world);
	XMVECTOR GetPositionVec() const;
	array<Bullet, 10>& GetBullets();

	bool IsActive() const;
	void Deactivate();

private:
	void UpdateExplosion(GameObject* world, float deltaTime);
	void UpdateRotation(float deltaTime);
	void UpdateMovement(float deltaTime);
	void UpdateBullets(float deltaTime);
	void UpdateReloadState(float deltaTime);

	void TryFire();

	XMFLOAT3 RandomDirection();

private:
	unique_ptr<VertexIndexBuffer> body = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> head = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> barrel = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> bullet = make_unique<VertexIndexBuffer>();

	GameObject eBody = {};
	GameObject eHead = {};
	GameObject eBarrel = {};
	GameObject eFiringPoint = {};

	const GameObject* eTarget = {};

	static constexpr float MIN_DISTANCE = 1.0f;

	enemyState eState = enemyState::None;

	bool bExploding = false;
	float explosionTime = 0.0f;
	std::vector<std::unique_ptr<GameObject>> sExplosions;
	std::vector<XMFLOAT3> explosionDirections;

	array<Bullet, 10> eBullets = {};
	int eBulletCount = 0;

	float eReloadTime = 2.5f;
	float eTimeInWait = 0.0f;
};

