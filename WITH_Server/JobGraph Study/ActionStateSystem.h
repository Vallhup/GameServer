#pragma once

#include "ECS.h"
#include "System.h"

class ActionStateSystem : public System {
public:
	ActionStateSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ActionStateSystem() = default;

	virtual void Execute(const float dT) override
	{
		auto& states = ecs.GetStorage<ActionState>();

		for (const auto& [entity, state] : states)
		{
			if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
			if (state.type != ActionType::None) continue;
			if (auto* intent = ecs.GetStorage<ActionIntent>().GetComponent(entity))
			{
				if (intent->attack)
				{
					state.type = ActionType::Attack;
					state.elapsed = 0.0f;
					state.duration = 1.0f;

					// TEMP : intent 초기화도 분리?
					intent->attack = false;
				}
			}
		}
	}

	virtual std::vector<std::type_index> WriteComponents() const
	{
		return { typeid(ActionIntent), typeid(ActionState) };
	}
};

