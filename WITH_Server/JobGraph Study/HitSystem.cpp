#include "pch.h"
#include "HitSystem.h"

void HitSystem::Execute(const float dT)
{
	// TODO : 현재는 frame 단위 처리만 가능, 추후 공격 단위 처리 추가 필요
	//        ex) 공격 판정이 여러 프레임에 걸쳐 발생하는 경우 등
	auto& hitEvents = ecs.GetStorage<HitTag>();

	std::vector<Entity> removeList;

	for (const auto& [entity, hitEvent] : hitEvents)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* actionState = ecs.GetStorage<ActionState>().GetComponent(entity);
		if (!actionState || actionState->type == ActionType::Dead) continue;
		if (auto* health = ecs.GetStorage<Health>().GetComponent(entity))
		{
			// TODO : guard 처리 필요
			health->current -= hitEvent.damage;

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
		hitEvents.RemoveComponent(entity);
	}
}

std::vector<std::type_index> HitSystem::ReadComponents() const
{
	return { typeid(ActionState) };
}

std::vector<std::type_index> HitSystem::WriteComponents() const
{
	return { typeid(HitTag), typeid(Health), typeid(ActionRequestTag) };
}

void HitSystem::RequestActionTransition(Entity entity, ActionType type)
{
	ecs.GetStorage<ActionRequestTag>().AddComponent(entity)->type = type;
}
