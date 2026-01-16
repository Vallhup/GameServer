#include "pch.h"
#include "HitApplySystem.h"

void HitApplySystem::Execute(const float dT)
{
	// TODO : 현재는 frame 단위 처리만 가능, 추후 공격 단위 처리 추가 필요
	//        ex) 공격 판정이 여러 프레임에 걸쳐 발생하는 경우 등
	auto& hits = ecs.GetStorage<HitTag>();

	std::vector<Entity> removeList;
	removeList.reserve(hits.Size());

	for (const auto& [entity, hit] : hits)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (hit.invalid) continue;

		auto* actionState = ecs.GetStorage<ActionState>().GetComponent(entity);
		if (!actionState || actionState->type == ActionType::Dead) continue;
		if (auto* health = ecs.GetStorage<Health>().GetComponent(entity))
		{
			// TODO : guard 처리 필요
			health->current -= hit.damage;

			if (health->current <= 0)
			{
				health->current = 0;
				RequestActionTransition(entity, ActionType::Dead);
			}

			else
			{
				RequestActionTransition(entity, ActionType::Hit);
			}
		}

		removeList.push_back(entity);
	}

	for (const auto& entity : removeList)
	{
		hits.RemoveComponent(entity);
	}
}

void HitApplySystem::RequestActionTransition(Entity entity, ActionType type)
{
	ecs.GetStorage<ActionRequestTag>().AddComponent(entity)->type = type;
}
