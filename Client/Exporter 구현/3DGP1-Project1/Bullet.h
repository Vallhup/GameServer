#pragma once
#include "GameObject.h"

class Mesh;
class VertexIndexBuffer;

class Bullet
{
public:
	Bullet() = default;
	Bullet(const Bullet&) = delete;
	Bullet& operator=(const Bullet&) = delete;
	~Bullet() = default;

	void Initialize(GameObject* world, VertexIndexBuffer* cube);
	void Update(float deltaTime, const float speed);
	void Release(GameObject* world);
	void Fire(const XMVECTOR& pos, const float rotY);

	bool IsActive() const;
	void Deactivate();

	const XMVECTOR GetPosition();

private:
	GameObject bBody = {};

	float lifeTime = 0.0f;
};

