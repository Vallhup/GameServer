#include "pch.h"
#include "BuffApplySystem.h"
#include "Tags.h"
#include "Bufs.h"
#include "BuffManager.h"


void BuffApplySystem::Execute(const double dT)
{
	auto events = _runtime.Events().Queue<DeathEvent>().ConsumeView();
	if (events.empty()) return;

	ECS& ecs = _runtime.GetECS();

	for (const auto& event : events)
	{
		if (!ecs.IsAlive(event.dead)) continue;

		if (rand() % 100 > 0)
		{
			std::vector<Entity> targets;
			FindTargetEntities(ecs, targets);

			// TODO : 버프 적용
			for (const Entity& target : targets)
			{
				// TODO : event.deadType에 따라 BuffType 결정
				BuffType type{ BuffType::None };
				if (event.deadType == EntityType::Final_Boss)
				{
					if (rand() % 100 > 50)
					{
						type = BuffType::HpBoost;
					}

					else
						type = BuffType::StaminaBoost;
				}

				GiveBuff(target, type);
			}
		}
	}
}

void BuffApplySystem::FindTargetEntities(const ECS& ecs, std::vector<Entity>& out)
{
	const auto& players = ecs.GetStorage<PlayerTag>();

	for (const auto& [entity, player] : players)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		out.push_back(entity);
	}
}

void BuffApplySystem::GiveBuff(Entity target, BuffType type)
{
	auto* bufComp = _runtime.GetECS().GetStorage<BuffsComp>().GetComponent(target);
	if (bufComp) return;

	const BuffDef& buf = BuffManager::Get().GetBuff(type);
	
	// TODO : buf type, policy, effect에 따라 bufComp에 적용

}


