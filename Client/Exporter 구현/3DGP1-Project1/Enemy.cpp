#include "pch.h"
#include "Enemy.h"
#include "Mesh.h"
#include "VertexIndexBuffer.h"
#include "Indexes.h"

void Enemy::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, GameObject* world, const GameObject* targetObject)
{
	body->Initialize(device, cmdList, CubeVertices(0.0f, 0.0f, 0.0f, 1.0f), CubeIndices());
	head->Initialize(device, cmdList, CubeVertices(0.33f, 0.33f, 0.33f, 1.0f), CubeIndices());
	barrel->Initialize(device, cmdList, CubeVertices(0.66f, 0.66f, 0.66f, 1.0f), CubeIndices());
	bullet->Initialize(device, cmdList, CubeVertices(1.0f, 0.0f, 0.0f, 1.0f), CubeIndices());

	eFiringPoint.GetTransform().SetPosition({ 0.0f, 0.0f, 0.06f });
	eBarrel.AddChild(&eFiringPoint);

	eBarrel.SetMesh(barrel.get());
	eBarrel.GetTransform().SetPosition({ 0.0f, 0.0f, 0.08f });
	eBarrel.GetTransform().SetScale({ 0.3f, 0.5f, 0.8f });
	eHead.AddChild(&eBarrel);

	eHead.SetMesh(head.get());
	eHead.GetTransform().SetPosition({ 0.0f, 0.075f, 0.0f });
	eHead.GetTransform().SetScale({ 0.4f, 0.5f, 0.6f });
	eBody.AddChild(&eHead);

	eBody.SetMesh(body.get());
	eBody.GetTransform().SetScale({ 1.3f, 0.6f, 1.8f });
	world->AddChild(&eBody);

	eTarget = targetObject;

	for (Bullet& Bullets : eBullets)
		Bullets.Initialize(world, bullet.get());
}

void Enemy::Update(GameObject* world, float deltaTime)
{
	if (eState == enemyState::Dying)
		UpdateExplosion(world, deltaTime);

	if (not eBody.IsActive() || eTarget == nullptr)
		return;
	
	UpdateRotation(deltaTime);
	UpdateMovement(deltaTime);
	UpdateBullets(deltaTime);
	UpdateReloadState(deltaTime);
	TryFire();
}

void Enemy::Release(GameObject* world)
{
	for (const auto& explosion : sExplosions)
	{
		if (world)
			world->RemoveChild(explosion.get());
	}
	sExplosions.clear();
	explosionDirections.clear();

	for (Bullet& bullet : eBullets)
		bullet.Release(world);  

	if (world)
		world->RemoveChild(&eBody);  

	eFiringPoint.Release(); 
	eBarrel.Release();
	eHead.Release();
	eBody.Release();

	eTarget = nullptr;
	eBulletCount = 0;
	explosionTime = 0.0f;
	eTimeInWait = 0.0f;
	eState = enemyState::None;
}

void Enemy::Fire()
{
	XMMATRIX eworld = eFiringPoint.GetTransform().CreateWorldMatrix()
		* eBarrel.GetTransform().CreateWorldMatrix()
		* eHead.GetTransform().CreateWorldMatrix()
		* eBody.GetTransform().CreateWorldMatrix();
	XMVECTOR startPosition = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), eworld);

	XMVECTOR headLookDirection = XMVector3TransformNormal(eHead.GetTransform().GetLookVec(), eBody.GetTransform().CreateBasisMatrix());
	float rotationY = GetAngleBetweenNormals(headLookDirection, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));

	if (XMVectorGetY(XMVector3Cross(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), headLookDirection)) < 0.0f)
	{
		rotationY *= -1.0f;
	}

	eBullets[eBulletCount].Fire(startPosition, rotationY);
	eBulletCount = (eBulletCount + 1) % static_cast<int>(eBullets.size());

	eState = enemyState::Reload;
}

void Enemy::SetPosition(const XMFLOAT3& pos)
{
	eBody.GetTransform().SetPosition(pos);
}

void Enemy::SetState(enemyState in)
{
	eState = in;
}

Enemy::enemyState Enemy::GetState()
{
	return eState;
}

BoundingBox Enemy::GetBoundingBox() const
{
	XMMATRIX world = eBody.GetTransform().CreateWorldMatrix();
	BoundingBox localBox(XMFLOAT3(0, 0, 0), XMFLOAT3(0.0325f, 0.15f, 0.045f));
	BoundingBox worldBox;
	localBox.Transform(worldBox, world);

	return worldBox;
}

void Enemy::GenerateExplosion(GameObject* world) {
	sExplosions.clear();
	explosionDirections.clear();

	for (int i = 0; i < 20; ++i) {
		auto obj = std::make_unique<GameObject>();
		Transform& transform = obj->GetTransform();
		transform.SetPosition(eBody.GetTransform().GetPosition());
		transform.SetRotation({ static_cast<float>(rand() % 360), static_cast<float>(rand() % 360), static_cast<float>(rand() % 360) });
		transform.SetScale({ 0.3f, 0.3f, 0.3f });
		obj->SetMesh(eBody.GetMesh());

		explosionDirections.push_back(RandomDirection());
		world->AddChild(obj.get());
		sExplosions.push_back(std::move(obj));
	}
}

XMVECTOR Enemy::GetPositionVec() const
{
	return eBody.GetTransform().GetPositionVec();
}

array<Bullet, 10>& Enemy::GetBullets()
{
	return eBullets;
}

bool Enemy::IsActive() const
{
	return eBody.GetActive();
}

void Enemy::Deactivate()
{
	eBody.SetActive(false);
	eHead.SetActive(false);
	eBarrel.SetActive(false);
	eFiringPoint.SetActive(false);

	for (Bullet& bullet : eBullets)
		bullet.Deactivate();

	eState = enemyState::Dying;
}

void Enemy::UpdateExplosion(GameObject* world, float deltaTime)
{
	explosionTime += deltaTime;

	for (size_t i = 0; i < sExplosions.size(); ++i) {
		Transform& transform = sExplosions[i]->GetTransform();
		XMVECTOR pos = XMLoadFloat3(&transform.GetPosition());
		XMVECTOR dir = XMLoadFloat3(&explosionDirections[i]);
		pos += dir * deltaTime * 0.5f;
		XMFLOAT3 newPos; XMStoreFloat3(&newPos, pos);
		transform.SetPosition(newPos);
	}

	if (explosionTime >= 1.0f) {
		for (const auto& explosion : sExplosions)
			world->RemoveChild(explosion.get());

		sExplosions.clear();
		explosionDirections.clear();
	}
}

void Enemy::UpdateRotation(float deltaTime)
{
	Transform& transform = eBody.GetTransform();
	XMVECTOR pos = transform.GetPositionVec();
	XMVECTOR targetPos = eTarget->GetTransform().GetPositionVec();

	XMVECTOR toTarget = targetPos - pos;
	XMVECTOR dir = XMVector3Normalize(toTarget);
	XMVECTOR look = transform.GetLookVec();

	float angleDiff = GetAngleBetweenNormals(look, dir);
	if (XMVectorGetY(XMVector3Cross(look, dir)) < 0.0f)
		angleDiff *= -1.0f;

	if (fabsf(angleDiff) > 1.0f)
	{
		float rotSpeed = deltaTime * 3.0f;
		transform.SetRotation({ 0.0f, transform.GetRotation().y + angleDiff * rotSpeed, 0.0f });
	}
}

void Enemy::UpdateMovement(float deltaTime)
{
	Transform& transform = eBody.GetTransform();
	XMVECTOR pos = transform.GetPositionVec();
	XMVECTOR targetPos = eTarget->GetTransform().GetPositionVec();

	float distance = XMVectorGetX(XMVector3Length(targetPos - pos));

	if (distance > MIN_DISTANCE)
	{
		XMVECTOR dir = XMVector3Normalize(targetPos - pos);
		float moveSpeed = deltaTime * 0.1f;
		transform.SetPositionVec(pos + dir * moveSpeed);
	}
}

void Enemy::UpdateBullets(float deltaTime)
{
	for (Bullet& bullet : eBullets)
	{
		bullet.Update(deltaTime, 0.4f);
	}
}

void Enemy::UpdateReloadState(float deltaTime)
{
	if (eState != enemyState::Reload)
		return;

	eTimeInWait += deltaTime;

	if (eTimeInWait >= eReloadTime)
	{
		eState = enemyState::None;
		eTimeInWait = 0.0f;
	}
}

void Enemy::TryFire()
{
	if (eState == enemyState::Reload)
		return;

	Transform& transform = eBody.GetTransform();
	XMVECTOR pos = transform.GetPositionVec();
	XMVECTOR targetPos = eTarget->GetTransform().GetPositionVec();
	XMVECTOR dir = XMVector3Normalize(targetPos - pos);
	XMVECTOR look = transform.GetLookVec();

	float angleDiff = GetAngleBetweenNormals(look, dir);
	if (XMVectorGetY(XMVector3Cross(look, dir)) < 0.0f)
		angleDiff *= -1.0f;

	float distance = XMVectorGetX(XMVector3Length(targetPos - pos));

	if (fabsf(angleDiff) <= 12.0f && distance <= MIN_DISTANCE)
	{
		Fire();
	}
}

XMFLOAT3 Enemy::RandomDirection()
{
	return {
		(rand() % 200 - 100) / 100.0f,
		(rand() % 200 - 100) / 100.0f,
		(rand() % 200 - 100) / 100.0f
	};
}