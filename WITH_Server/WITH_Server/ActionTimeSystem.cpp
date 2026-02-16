#include "pch.h"
#include "ActionTimeSystem.h"

void ActionTimeSystem::Execute(const double dT)
{
	auto& actions = _runtime.GetECS().GetStorage<ActionState>();

	for (const auto& [entity, action] : actions)
	{
		if (_runtime.GetECS().GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (action.type == ActionType::None) continue;

		action.elapsed += dT;
	}
}