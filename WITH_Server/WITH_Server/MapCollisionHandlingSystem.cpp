#include "pch.h"
#include "MapCollisionHandlingSystem.h"
#include "Framework.h"

void MapCollisionHandlingSystem::Execute(const float dT)
{
	auto& events = ecs.mapCollisionEvents;
	if (events.empty()) return;

	std::sort(events.begin(), events.end(),
		[](const MapCollisionEvent& a, const MapCollisionEvent& b)
		{
			return a.entity < b.entity;
		});

	auto& transforms = ecs.GetStorage<Transform>();

	for (size_t i = 0; i < events.size();)
	{
		const Entity entity = events[i].entity;

		// 중복 구간 찾기
		size_t j = i + 1;
		while (j < events.size() && events[j].entity.id == entity.id) ++j;

		auto* transform = transforms.GetComponent(entity);
		if (!transform) { i = j; continue; }

		// TODO : 필요하면 penetration이 큰 순서대로 처리
		//        + 최대 보정값 설정

		XMVECTOR corr = XMVectorZero();

		for (size_t k = i; k != j; ++k)
		{
			const XMVECTOR normal = XMLoadFloat3(&events[k].normal);
			const float pen = events[k].penetration;

			const float resolved = XMVectorGetX(
				XMVector3Dot(corr, normal));
			const float remaining = pen - resolved;
			if (remaining <= 0.0f) continue;

			corr = XMVectorAdd(corr,
				XMVectorScale(normal, remaining));
		}

		XMFLOAT3 corrF{ 0, 0, 0 };
		XMStoreFloat3(&corrF, corr);

		const float corrSq = corrF.x * corrF.x
			+ corrF.y * corrF.y + corrF.z * corrF.z;

		if (corrSq > 1e-12f)
		{
			XMVECTOR pos = XMLoadFloat3(&transform->position);
			pos = XMVectorAdd(pos, corr);
			XMStoreFloat3(&transform->position, pos);

			Framework::Get().outEventQueue.push(OutputEvent{
				entity, DirtyType::Moved });
		}

		i = j;
	}

	events.clear();
}