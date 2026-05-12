#include "pch.h"
#include "TrailComponent.h"
#include "Shader.h"
#include "GameObject.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Transform.h"

PSOType TrailComponent::GetPSOType() const { return PSOType::Trail; }

void TrailComponent::Update(float deltaTime)
{
	auto owner = GetGameObject();
	if (owner)
	{
		auto animMachine = owner->GetComponent<AnimationMachine>();
		bool isAttacking = animMachine && (animMachine->IsPlaying("AttackCombo1") || animMachine->IsPlaying("AttackCombo2") || animMachine->IsPlaying("AttackCombo3"));
		SetActive(isAttacking);

		if (isAttacking)
		{
			auto animator = owner->GetComponent<Animator>();
			auto transform = owner->GetComponent<Transform>();

			if (animator && animator->IsInitialized() && transform)
			{
				XMFLOAT3 bonePos = animator->GetBonePosition(45);
				XMVECTOR boneRotQuat = animator->GetBoneRotation(45);

				XMMATRIX worldMat = transform->GetWorldMatrix();
				XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

				XMVECTOR offsetRot = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2);
				XMFLOAT3 playerRot = transform->GetRotation();
				XMVECTOR playerRotQuat = XMQuaternionRotationRollPitchYaw(playerRot.x, playerRot.y, playerRot.z);

				XMVECTOR finalRotQuat = XMQuaternionMultiply(offsetRot, boneRotQuat);
				finalRotQuat = XMQuaternionMultiply(finalRotQuat, playerRotQuat);
				XMMATRIX rotMat = XMMatrixRotationQuaternion(finalRotQuat);

				XMVECTOR swordDir = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
				swordDir = XMVector3Normalize(swordDir);

				float bladeLength = 1.0f;
				XMVECTOR topPos = XMVectorAdd(worldPos, XMVectorScale(swordDir, bladeLength));

				XMFLOAT3 top, bottom;
				XMStoreFloat3(&top, topPos);
				XMStoreFloat3(&bottom, worldPos);

				AddPoint(top, bottom);
			}
		}
	}

	if (!isActive && points.empty()) return;

	for (auto& pt : points)
	{
		pt.age += deltaTime;
	}

	while (!points.empty() && points.front().age >= maxLifetime)
	{
		points.erase(points.begin());
	}
}

void TrailComponent::AddPoint(const XMFLOAT3& top, const XMFLOAT3& bottom)
{
	if (!isActive) return;

	TrailPoint newPoint;
	newPoint.top = top;
	newPoint.bottom = bottom;
	newPoint.age = 0.0f;

	if (!points.empty())
	{
		const auto& last = points.back();
		float dx = top.x - last.top.x;
		float dy = top.y - last.top.y;
		float dz = top.z - last.top.z;
		float distSq = dx * dx + dy * dy + dz * dz;

		if (distSq < 0.0001f)
			return;
	}

	if (points.size() >= maxElements)
	{
		points.erase(points.begin());
	}

	points.push_back(newPoint);
}

void TrailComponent::SetActive(bool active)
{
	isActive = active;
}

void TrailComponent::Clear()
{
	points.clear();
	vertices.clear();
	indices.clear();
}

void TrailComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	if (points.size() < 2)
	{
		vertices.clear();
		indices.clear();
		return;
	}

	vertices.clear();
	indices.clear();

	for (size_t i = 0; i < points.size(); ++i)
	{
		const auto& pt = points[i];
		float t = static_cast<float>(i) / static_cast<float>(points.size() - 1);
		float alpha = 1.0f - (pt.age / maxLifetime);
		alpha = max(0.0f, alpha);

		EffectVertex topVert;
		topVert.position = pt.top;
		topVert.uv = { t, 0.0f };
		topVert.alpha = alpha;
		vertices.push_back(topVert);

		EffectVertex bottomVert;
		bottomVert.position = pt.bottom;
		bottomVert.uv = { t, 1.0f };
		bottomVert.alpha = alpha;
		vertices.push_back(bottomVert);
	}

	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		UINT16 topLeft = static_cast<UINT16>(i * 2);
		UINT16 bottomLeft = static_cast<UINT16>(i * 2 + 1);
		UINT16 topRight = static_cast<UINT16>((i + 1) * 2);
		UINT16 bottomRight = static_cast<UINT16>((i + 1) * 2 + 1);

		indices.push_back(topLeft);
		indices.push_back(topRight);
		indices.push_back(bottomLeft);
		indices.push_back(bottomLeft);
		indices.push_back(topRight);
		indices.push_back(bottomRight);
	}
}
