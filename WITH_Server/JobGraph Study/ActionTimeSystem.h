#pragma once

#include "ECS.h"
#include "System.h"

class ActionTimeSystem : public System {
public:
	ActionTimeSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~ActionTimeSystem() = default;

	virtual void Execute(const float dT) override
	{
		auto& actions = ecs.GetStorage<ActionState>();

		for (const auto& [entity, action] : actions)
		{
			if (action.type == ActionType::None) continue;

			action.elapsed += dT;

			// TEMP : 상태 전이 책임 분리
			if (action.elapsed >= action.duration)
			{
				action.type = ActionType::None;
				action.elapsed = 0.0f;
				action.duration = 0.0f;
			}
		}
	}

	virtual std::vector<std::type_index> WriteComponents() const
	{
		return { typeid(ActionState) };
	}
};

