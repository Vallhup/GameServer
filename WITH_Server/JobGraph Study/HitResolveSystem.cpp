#include "pch.h"
#include "HitResolveSystem.h"

void HitResolveSystem::Execute(const float dT)
{
    auto& hits = ecs.GetStorage<HitTag>();
    auto& states = ecs.GetStorage<ActionState>();

    for (const auto& [entity, hit] : hits)
    {
        auto* state = states.GetComponent(entity);
        if (!state)
            continue;

        // TEMP : Parry 판정 윈도우 결정 필요
        static constexpr float PARRY_WINDOW = 0.2f;

        // Parry 성공 조건
        if (state->type == ActionType::Parry &&
            state->elapsed <= PARRY_WINDOW)
        {
            // Hit 무효화
            hit.invalid = true;

            // Parry 성공
            ecs.GetStorage<ParryBuff>().AddComponent(entity);
            ecs.GetStorage<ActionRequestTag>().AddComponent(entity)->type = ActionType::Stun;
        }
    }
}

std::vector<std::type_index> HitResolveSystem::ReadComponents() const
{
	return { typeid(ActionState) };
}

std::vector<std::type_index> HitResolveSystem::WriteComponents() const
{
	return { typeid(HitTag), typeid(ActionRequestTag), typeid(ParryBuff) };
}