#include "pch.h"
#include "Bullet.h"
#include "Mesh.h"
#include "VertexIndexBuffer.h"

void Bullet::Initialize(GameObject* world, VertexIndexBuffer* cube)
{
	bBody.SetMesh(cube);
	bBody.GetTransform().SetScale({ 0.2f, 0.2f, 0.8f });
	world->AddChild(&bBody);
}

void Bullet::Update(float deltaTime, const float speed)
{
	if (not bBody.IsActive()) return;

	lifeTime -= deltaTime;
	if (lifeTime <= 0.0f)
	{
		bBody.SetActive(false);
		return;
	}

	Transform& transform = bBody.GetTransform();
	XMVECTOR velocity = transform.GetLookVec() * speed * deltaTime;
	transform.SetPositionVec(transform.GetPositionVec() + velocity);
}

void Bullet::Release(GameObject* world)
{
	if (world)
		world->RemoveChild(&bBody);

	bBody.Release();

	lifeTime = 0.0f;
}

void Bullet::Fire(const XMVECTOR& pos, const float rotY)
{
	Transform& transform = bBody.GetTransform();
	transform.SetPositionVec(pos);
	transform.SetRotation({ 0.0f, rotY, 0.0f });
	bBody.SetActive(true);
	lifeTime = 4.0f;
}

bool Bullet::IsActive() const
{
	return bBody.GetActive();
}

void Bullet::Deactivate()
{
	bBody.SetActive(false);
}

const XMVECTOR Bullet::GetPosition()
{
	return bBody.GetTransform().GetPositionVec();
}
