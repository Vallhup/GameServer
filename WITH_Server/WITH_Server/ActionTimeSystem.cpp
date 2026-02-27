#include "pch.h"
#include "ActionTimeSystem.h"
#include "Tags.h"

void ActionTimeSystem::Execute(const double dT)
{
	auto& actions = _runtime.GetECS().GetStorage<ActionState>();

	for (const auto& [entity, action] : actions)
	{
		if (_runtime.GetECS().GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (action.action == ActionType::None) continue;

		action.elapsed += dT;
		action.progress = (action.duration > 0.0) ?
			std::clamp(action.elapsed / action.duration, 0.0, 1.0) : 1.0;
	}
}