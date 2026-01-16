#include "pch.h"
#include "ActionTimeSystem.h"

void ActionTimeSystem::Execute(const float dT)
{
	auto& actions = ecs.GetStorage<ActionState>();

	for (const auto& [entity, action] : actions)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (action.type == ActionType::None) continue;

		action.elapsed += dT;
		if (action.elapsed >= action.duration)
		{
			auto request = ecs.GetStorage<ActionRequestTag>().AddComponent(entity);
			request->type = ActionType::None;
		}
	}
}

std::vector<std::type_index> ActionTimeSystem::WriteComponents() const
{
	return { typeid(ActionState) };
}