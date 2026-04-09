#include "pch.h"
#include "EntityManager.h"

Entity EntityManager::Reserve()
{
	Entity e = AcquireFreeOrNewSlot();
	_states[e.id] = EntitySlotState::Reserved;
	return e;
}

bool EntityManager::MaterializeReserved(Entity e)
{
	if (GetHandleStatus(e) != EntityHandleStatus::Reserved)
		return false;

	_states[e.id] = EntitySlotState::Alive;
	return TryAppendAlive(e);
}

bool EntityManager::DiscardReserved(Entity e)
{
	if (GetHandleStatus(e) != EntityHandleStatus::Reserved)
		return false;

	ReleaseSlot(e);
	return true;
}

bool EntityManager::Destroy(Entity e)
{
	if (GetHandleStatus(e) != EntityHandleStatus::Alive)
		return false;

	if (!TryRemoveAlive(e))
		return false;

	ReleaseSlot(e);
	return true;
}

EntityHandleStatus EntityManager::GetHandleStatus(Entity e) const
{
	if (e.IsNull())
		return EntityHandleStatus::Invalid;

	if (!IsSlotIndexValid(e.id))
		return EntityHandleStatus::Invalid;

	if (!IsCurrentGeneration(e))
		return EntityHandleStatus::Invalid;

	switch (_states[e.id]) {
	case EntitySlotState::Free:
		return EntityHandleStatus::Free;
	case EntitySlotState::Reserved:
		return EntityHandleStatus::Reserved;
	case EntitySlotState::Alive:
		return EntityHandleStatus::Alive;
	default:
		assert(false && "Unknown EntitySlotState.");
		return EntityHandleStatus::Invalid;
	}
}

bool EntityManager::Exists(Entity e) const
{
	const EntityHandleStatus status = GetHandleStatus(e);
	return status == EntityHandleStatus::Reserved ||
		status == EntityHandleStatus::Alive;
}

bool EntityManager::IsReserved(Entity e) const
{
	return GetHandleStatus(e) == EntityHandleStatus::Reserved;
}

bool EntityManager::IsAlive(Entity e) const
{
	return GetHandleStatus(e) == EntityHandleStatus::Alive;
}

void EntityManager::Clear()
{
	_generations.clear();
	_states.clear();
	_freeIds.clear();
	_alive.clear();
	_aliveIndex.clear();
}

bool EntityManager::IsSlotIndexValid(int id) const
{
	return id >= 0 && static_cast<size_t>(id) < _generations.size();
}

bool EntityManager::IsCurrentGeneration(Entity e) const
{
	return IsSlotIndexValid(e.id) && _generations[e.id] == e.generation;
}

Entity EntityManager::AcquireFreeOrNewSlot()
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
		_states.push_back(EntitySlotState::Free);
		_aliveIndex.push_back(-1);
	}

	assert(_states[id] == EntitySlotState::Free);
	assert(_aliveIndex[id] == -1);

	return Entity{ id, _generations[id] };
}

bool EntityManager::TryAppendAlive(Entity e)
{
	if (!IsCurrentGeneration(e))
		return false;

	if (_aliveIndex[e.id] != -1)
		return false;

	_aliveIndex[e.id] = static_cast<int>(_alive.size());
	_alive.push_back(e);
	return true;
}

bool EntityManager::TryRemoveAlive(Entity e)
{
	if (!IsCurrentGeneration(e))
		return false;

	const int idx = _aliveIndex[e.id];
	if (idx < 0 || static_cast<size_t>(idx) >= _alive.size())
		return false;

	if (_alive[idx] != e)
		return false;

	const int last = static_cast<int>(_alive.size() - 1);
	if (idx != last)
	{
		Entity moved = _alive[last];
		_alive[idx] = moved;
		_aliveIndex[moved.id] = idx;
	}

	_alive.pop_back();
	_aliveIndex[e.id] = -1;
	return true;
}

void EntityManager::ReleaseSlot(Entity e)
{
	assert(IsCurrentGeneration(e));
	assert(_states[e.id] == EntitySlotState::Reserved || _states[e.id] == EntitySlotState::Alive);
	assert(_aliveIndex[e.id] == -1);

	_states[e.id] = EntitySlotState::Free;
	++_generations[e.id];
	_freeIds.push_back(e.id);
}
