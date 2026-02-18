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
		_aliveIndex.push_back(-1);
	}

	Entity e{ id, _generations[id] };

	_aliveIndex[id] = static_cast<int>(_alive.size());
	_alive.push_back(e);

	_aliveCount = _alive.size();
	return e;
}

bool EntityManager::Destroy(Entity e)
{
	if (!IsAlive(e)) return false;

	const int id = e.id;
	int idx = _aliveIndex[id];

	const int last = static_cast<int>(_alive.size() - 1);
	if (idx != last)
	{
		Entity moved = _alive[last];
		_alive[idx] = moved;
		_aliveIndex[moved.id] = idx;
	}

	_alive.pop_back();
	_aliveIndex[id] = -1;

	++_generations[id];
	_freeIds.push_back(id);

	_aliveCount = _alive.size();
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