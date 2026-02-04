#include "pch.h"
#include "EntityManager.h"
#include <iostream>

Entity EntityManager::Create()
{
	int id;

	if (!_freeIds.empty())
	{
		id = _freeIds.back();
		_freeIds.pop_back();
	}

	else
	{
		if (_generations.size() >= size_t(std::numeric_limits<int>::max()))
			throw std::runtime_error("EntityManager: Entity id Overflow.");

		id = static_cast<int>(_generations.size());
		_generations.push_back(0);
	}

	++_aliveCount;
	return Entity{ id, _generations[id] };
}

bool EntityManager::Destroy(Entity e)
{
	if (!IsAlive(e)) return false;

	++_generations[e.id];
	_freeIds.push_back(e.id);

	--_aliveCount;
	return true;
}

bool EntityManager::IsAlive(Entity e) const
{
	if (e.IsNull())
		return false;

	if (e.id >= _generations.size())
		return false;

	return _generations[e.id] == e.generation;
}