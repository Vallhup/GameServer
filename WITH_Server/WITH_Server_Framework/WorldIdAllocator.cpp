#include "pch.h"
#include "WorldIdAllocator.h"
#include "WorldRegistry.h"

WorldIdAllocator::WorldIdAllocator(uint32 reserve)
{
	_gens.reserve(reserve);
	_gens.push_back(0);
}

WorldId WorldIdAllocator::Allocate()
{
	uint32 id{ 0 };

	if (!_freeIds.empty())
	{
		id = _freeIds.back();
		assert(id < _gens.size());

		_freeIds.pop_back();

		uint16 gen = _gens[id];
		assert(gen != 0);

		return WorldId::Create(id, gen);
	}

	else
	{
		id = static_cast<uint32>(_gens.size());
		_gens.push_back(1);

		return WorldId::Create(id, 1);
	}
}

void WorldIdAllocator::Free(WorldId worldId)
{
	if (!worldId.IsValid()) return;

	const uint32 id = worldId.GetId();
	const uint16 gen = worldId.GetGen();

	if (id == 0 || id >= _gens.size()) return;
	if (_gens[id] != gen) return;

	uint16_t next = static_cast<uint16>(_gens[id] + 1);
	assert(next != 0);

	_gens[id] = next;
	_freeIds.push_back(id);
}

bool WorldIdAllocator::IsAlive(WorldId worldId) const
{
	if (!worldId.IsValid()) return false;

	const uint32 id = worldId.GetId();
	const uint16 gen = worldId.GetGen();

	const bool isAlive =
		(id != 0) &&
		(id < _gens.size()) &&
		(_gens[id] == gen);

	return isAlive;
}

void WorldIdAllocator::Clear()
{
	_gens.clear();
	_freeIds.clear();

	_gens.push_back(0);
}
