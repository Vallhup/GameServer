#include "pch.h"
#include "World.h"
#include "EventRegistry.h"

World::World(WorldId id, const WorldDesc& desc, ThreadPool& pool, std::unique_ptr<IWorldImpl> impl)
	: _id(id), _desc(desc), _runtime(pool, *impl),
	_impl(std::move(impl))
{
	//RegisterWorldEvents(_runtime.Events(), desc);
}

void World::Init()
{
	WorldBuilder builder{ _runtime, _desc };
	_impl->Configure(builder);
	builder.Commit();
	_runtime.BuildGraph();
}

void World::Update(const double dT)
{
	_runtime.Run(dT);
	FinalizePresence();
}

void World::Shutdown()
{
	_impl->OnShutdown(_runtime);
}

Entity World::RequestSpawnPlayer(uint32_t connId)
{
	auto it = _players.find(connId);
	if (it != _players.end() &&
		it->second.state != PlayerPresenceState::None)
	{
		return Entity{};
	}

	Entity entity = _impl->SpawnPlayer(_runtime, connId);
	if (entity.IsNull())
		return Entity{};

	PlayerPresenceEntry entry;
	entry.state = PlayerPresenceState::PendingSpawn;
	entry.entity = entity;

	_players[connId] = entry;
	return entity;
}

bool World::RequestDespawnPlayer(uint32_t connId)
{
	auto it = _players.find(connId);
	if (it == _players.end()) return false;
	if (it->second.state != PlayerPresenceState::Active) return false;

	if (!_impl->DespawnPlayer(_runtime, connId)) return false;

	it->second.state = PlayerPresenceState::PendingDespawn;
	return true;
}

bool World::TryMakeSnapshot(WorldRuntime& rt, uint32 connId, PlayerSnapshot& out)
{
	return _impl->TryMakeSnapshot(rt, connId, out);
}

bool World::ApplySnapshot(WorldRuntime& rt, uint32 connId, const PlayerSnapshot& snapshot)
{
	return _impl->ApplySnapshot(rt, connId, snapshot);
}

void World::FinalizePresence()
{
	for (auto it = _players.begin(); it != _players.end();)
	{
		auto& [connId, entry] = *it;

		switch (entry.state) {
		case PlayerPresenceState::PendingSpawn:
		{
			// TEMP : 실제 Component 등 완전히 Setting 됬는지 확인
			entry.state = PlayerPresenceState::Active;
			++it;
			break;
		}
		case PlayerPresenceState::PendingDespawn:
		{
			it = _players.erase(it);
			break;
		}
		default:
		{
			++it;
			break;
		}
		}
	}
}

bool World::HasPresence(uint32_t connId) const
{
	auto it = _players.find(connId);
	return
		it != _players.end() &&
		it->second.state != PlayerPresenceState::None;
}

bool World::HasPlayer(uint32_t connId) const
{
	auto it = _players.find(connId);
	return
		it != _players.end() &&
		it->second.state != PlayerPresenceState::Active;
}

bool World::IsPlayerPendingSpawn(uint32_t connId) const
{
	auto it = _players.find(connId);
	return
		it != _players.end() &&
		it->second.state != PlayerPresenceState::PendingSpawn;
}

bool World::IsPlayerPendingDespawn(uint32_t connId) const
{
	auto it = _players.find(connId);
	return
		it != _players.end() &&
		it->second.state != PlayerPresenceState::PendingDespawn;
}