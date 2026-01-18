#include "pch.h"
#include "ActionStateSystem.h"
#include "ActionManager.h"

ActionStateSystem::ActionStateSystem(ECS& e, int p) : System(e, p) 
{
}

void ActionStateSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<ActionState>();
	auto& intents = ecs.GetStorage<ActionIntent>();

	for (const auto& [entity, state] : states)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (state.type != ActionType::None) continue;

		if (auto* intent = intents.GetComponent(entity))
		{
			if (intent->parry)
			{
				ecs.actionRequestEvents.emplace_back(entity, ActionType::Parry);
				intent->parry = false;
			}

			if (intent->dodge)
			{
				ecs.actionRequestEvents.emplace_back(entity, ActionType::Dodge);
				intent->dodge = false;
			}

			if (intent->attack)
			{
				ecs.actionRequestEvents.emplace_back(entity, ActionType::Attack);
				intent->attack = false;
			}
		}
	}
}
