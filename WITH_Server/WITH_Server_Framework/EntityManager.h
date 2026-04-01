#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "Entity.h"

enum class EntitySlotState : uint8_t
{
	Free,
	Reserved,
	Alive
};

enum class EntityHandleStatus : uint8_t
{
	Invalid,
	Free,
	Reserved,
	Alive
};

class EntityManager {
public:
	Entity Reserve();
	bool MaterializeReserved(Entity e);
	bool DiscardReserved(Entity e);
	bool Destroy(Entity e);

	EntityHandleStatus GetHandleStatus(Entity e) const;
	bool Exists(Entity e) const;
	bool IsReserved(Entity e) const;
	bool IsAlive(Entity e) const;

	std::span<const Entity> AliveEntities() const { return _alive; }
	void Clear();

private:
	bool IsSlotIndexValid(int id) const;
	bool IsCurrentGeneration(Entity e) const;

	Entity AcquireFreeOrNewSlot();
	bool TryAppendAlive(Entity e);
	bool TryRemoveAlive(Entity e);
	void ReleaseSlot(Entity e);

private:
	std::vector<int> _generations;
	std::vector<EntitySlotState> _states;
	std::vector<int> _freeIds;

	std::vector<Entity> _alive;
	std::vector<int> _aliveIndex;
};

