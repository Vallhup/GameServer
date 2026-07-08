#include "pch.h"
#include "TrailComponent.h"
#include "Shader.h"
#include "GameObject.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Transform.h"

PSOType TrailComponent::GetPSOType() const { return PSOType::Trail; }

void TrailComponent::SetBoneIndices(const vector<int>& indices)
{
	if (indices.empty()) return;

	trails.clear();
	trails.reserve(indices.size());
	for (int idx : indices)
	{
		TrailStrip strip;
		strip.boneIndex = idx;
		trails.push_back(strip);
	}
}

void TrailComponent::Update(float deltaTime)
{
	auto owner = GetGameObject();
	if (owner)
	{
		auto animMachine = owner->GetComponent<AnimationMachine>();
		bool isAttacking = false;
		if (animMachine)
			for (const auto& clip : activationClips)
				if (animMachine->IsPlaying(clip)) { isAttacking = true; break; }
		SetActive(isAttacking);

		if (isAttacking)
		{
			auto animator = owner->GetComponent<Animator>();
			auto transform = owner->GetComponent<Transform>();

			if (animator && animator->IsInitialized() && transform)
			{
				XMMATRIX worldMat = transform->GetWorldMatrix();

				XMVECTOR offsetRot = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2);
				XMFLOAT3 playerRot = transform->GetRotation();
				XMVECTOR playerRotQuat = XMQuaternionRotationRollPitchYaw(playerRot.x, playerRot.y, playerRot.z);

				for (auto& strip : trails)
				{
					XMFLOAT3 bonePos = animator->GetBonePosition(strip.boneIndex);
					XMVECTOR boneRotQuat = animator->GetBoneRotation(strip.boneIndex);

					XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

					XMVECTOR finalRotQuat = XMQuaternionMultiply(offsetRot, boneRotQuat);
					finalRotQuat = XMQuaternionMultiply(finalRotQuat, playerRotQuat);
					XMMATRIX rotMat = XMMatrixRotationQuaternion(finalRotQuat);

					XMVECTOR swordDir = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
					swordDir = XMVector3Normalize(swordDir);

					XMVECTOR topPos = XMVectorAdd(worldPos, XMVectorScale(swordDir, bladeLength));

					XMFLOAT3 top, bottom;
					XMStoreFloat3(&top, topPos);
					XMStoreFloat3(&bottom, worldPos);

					AddPoint(strip, top, bottom);
				}
			}
		}
	}

	if (!isActive && !HasPoints()) return;

	for (auto& strip : trails)
	{
		for (auto& pt : strip.points)
			pt.age += deltaTime;

		while (!strip.points.empty() && strip.points.front().age >= maxLifetime)
			strip.points.erase(strip.points.begin());
	}
}

void TrailComponent::AddPoint(TrailStrip& strip, const XMFLOAT3& top, const XMFLOAT3& bottom)
{
	if (!isActive) return;

	if (!strip.points.empty())
	{
		const auto& last = strip.points.back();
		float dx = top.x - last.top.x;
		float dy = top.y - last.top.y;
		float dz = top.z - last.top.z;
		float distSq = dx * dx + dy * dy + dz * dz;

		if (distSq < 0.0001f)
			return;
	}

	// maxElements를 트레일 줄기 수로 나눠 정점 버퍼(maxElements*4) 한도 내로 유지
	const size_t capacity = max<size_t>(2, maxElements / trails.size());
	if (strip.points.size() >= capacity)
		strip.points.erase(strip.points.begin());

	TrailPoint newPoint;
	newPoint.top = top;
	newPoint.bottom = bottom;
	newPoint.age = 0.0f;
	strip.points.push_back(newPoint);
}

void TrailComponent::SetActive(bool active)
{
	isActive = active;
}

bool TrailComponent::HasPoints() const
{
	for (const auto& strip : trails)
		if (!strip.points.empty())
			return true;
	return false;
}

void TrailComponent::Clear()
{
	for (auto& strip : trails)
		strip.points.clear();

	vertices.clear();
	indices.clear();
}

void TrailComponent::BuildMesh(const XMFLOAT3& cameraPos)
{
	vertices.clear();
	indices.clear();

	for (const auto& strip : trails)
	{
		const auto& points = strip.points;
		if (points.size() < 2)
			continue;

		const UINT16 base = static_cast<UINT16>(vertices.size());

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
			UINT16 topLeft = base + static_cast<UINT16>(i * 2);
			UINT16 bottomLeft = base + static_cast<UINT16>(i * 2 + 1);
			UINT16 topRight = base + static_cast<UINT16>((i + 1) * 2);
			UINT16 bottomRight = base + static_cast<UINT16>((i + 1) * 2 + 1);

			indices.push_back(topLeft);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);
			indices.push_back(bottomRight);
		}
	}
}
