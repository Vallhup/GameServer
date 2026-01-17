#include "pch.h"
#include "ColliderUpdateSystem.h"
#include "Math.h"

void ColliderUpdateSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& colliders = ecs.GetStorage<Collider>();

	for (const auto& [entity, collider] : colliders)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* trans = transforms.GetComponent(entity);
		if (!trans) continue;
		if (!collider.staticDatas) continue;

		const size_t n = collider.staticDatas->size();

#ifdef _DEBUG
		assert(collider.localDatas.size() == n);
		assert(collider.worldDatas.size() == n);
#endif
		XMMATRIX world = TransformHelper::ToMatrix(*trans);

		for (size_t i = 0; i < n; ++i)
		{
			const auto& localData = collider.localDatas[i];

			XMVECTOR p0 = XMVector3Transform(localData.P0(), world);
			XMVECTOR p1 = XMVector3Transform(localData.P1(), world);

			XMStoreFloat3(&collider.worldDatas[i].p0, p0);
			XMStoreFloat3(&collider.worldDatas[i].p1, p1);
		}
	}
}