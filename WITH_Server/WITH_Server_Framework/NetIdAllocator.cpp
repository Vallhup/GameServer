#include "pch.h"
#include "NetIdAllocator.h"
#include "NetIdRegistry.h"

NetIdAllocator::NetIdAllocator(uint32 reserve)
{
	_gens.reserve(reserve);
	_gens.push_back(0);
}

NetId NetIdAllocator::Allocate()
{
	uint32 id{ 0 };

	if (!_freeIds.empty())
	{
		id = _freeIds.back();
		assert(id < _gens.size());

		_freeIds.pop_back();

		uint16_t gen = _gens[id];
		assert(gen != 0);

		return NetId::Create(id, gen);
	}

	else
	{
		id = static_cast<uint32>(_gens.size());
		_gens.push_back(1);
		
		return NetId::Create(id, 1);
	}
}

void NetIdAllocator::Free(NetId netId)
{
	if (!netId.IsValid()) return;

	const uint32 id = netId.GetId();
	const uint16 gen = netId.GetGen();

	if (id == 0 || id >= _gens.size()) return;
	if (_gens[id] != gen) return;

	uint16_t next = static_cast<uint16>(_gens[id] + 1);
	assert(next != 0);

	_gens[id] = next;
	_freeIds.push_back(id);
}

bool NetIdAllocator::IsAlive(NetId netId) const
{
	if (!netId.IsValid()) return false;

	const uint32 id = netId.GetId();
	const uint16 gen = netId.GetGen();

	const bool isAlive =
		(id != 0) &&
		(id < _gens.size()) &&
		(_gens[id] == gen);

	return isAlive;
}