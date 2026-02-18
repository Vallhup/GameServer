#pragma once

#include <span>
#include "Entity.h"

class EntityManager {
public:
	Entity Create();
	bool Destroy(Entity e);
	bool IsAlive(Entity e) const;

	std::span<const Entity> AliveEntities() const { return _alive; }

private:
	std::vector<int> _generations;
	std::vector<int> _freeIds;

	std::vector<Entity> _alive;
	std::vector<int> _aliveIndex;

	size_t _aliveCount{ 0 };
};

