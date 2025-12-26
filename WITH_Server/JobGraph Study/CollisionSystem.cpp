#include "CollisionSystem.h"
#include "Collision.h"

void CollisionSystem::Execute(const float dT)
{
	auto& colliders = ecs.GetStorage<Collider>();
	auto& transforms = ecs.GetStorage<Transform>();

	std::vector<Entity> entities;
	entities.reserve(colliders.end().i);
	for (const auto& [entity, _] : colliders)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		entities.push_back(entity);
	}

	for (size_t i = 0; i < entities.size(); ++i)
	{
		auto* colA = colliders.GetComponent(entities[i]);
		if (!colA) continue;

		for (size_t j = i + 1; j < entities.size(); ++j)
		{
			auto* colB = colliders.GetComponent(entities[j]);
			if (!colB) continue;

			bool collided{ false };
			for (const auto& cA : colA->worldCapsules)
			{
				for (const auto& cB : colB->worldCapsules)
				{
					if (Collision::CheckCapsuleVsCapsule(cA, cB))
					{
						// TODO : 충돌 로직
						collided = true;
						break;
					}
				}
			}

#ifdef _DEBUG
			if (collided)
			{
				printf("Collision! %2u vs %2u\n",
					entities[i].id, entities[j].id);
			}
#endif
		}
	}
}