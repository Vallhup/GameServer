#include "pch.h"
#include "WorldService.h"
#include "WorldRegistry.h"

bool WorldService::InitSquare()
{
	if (_squareWorldId.IsValid()) return false;

	WorldDesc desc
	{
		.type = WorldType::Square,
		.worldRulesetId = 0,
		.Capacity = 5000,
		.mapId = 0,
		.netPolicy = 0,
	};
	_squareWorldId = _reg.CreateWorld(desc);
	if (_squareWorldId.IsValid())
	{
		if (_scheduler)
			_scheduler->Register(_squareWorldId, 30);

#ifdef _DEBUG
		else
			throw std::runtime_error("스케줄러 초기화 누락");
#endif

		return false;
	}

	return true;
}

WorldId WorldService::ResolveTargetWorld(WorldType type, uint64 key)
{
	assert(_squareWorldId.IsValid());

	if (type == WorldType::Square)
		return _squareWorldId;
	
	if (key != 0)
	{
		InstKey instKey{ type, key };
		auto it = _resolved.find(instKey);
		if (it != _resolved.end())
		{
			WorldId id = it->second;
			if (id.IsValid() && _reg.IsAlive(id))
				return id;

			_resolved.erase(it);
		}
	}

	WorldId id = CreateWorld(type, key);
	if (id.IsValid() && key != 0)
		_resolved.try_emplace(InstKey{ type, key }, id);

	return id;
}

void WorldService::OnPlayerEnter(WorldId worldId, uint32 count)
{
	if (worldId == _squareWorldId) return;

	auto it = _instances.find(worldId);
	if (it == _instances.end()) return;

	it->second.playerCount += count;
	it->second.pendingDestroy = false;
}

void WorldService::OnPlayerLeave(WorldId worldId, uint32 count)
{
	if (worldId == _squareWorldId) return;

	auto it = _instances.find(worldId);
	if (it == _instances.end()) return;

	it->second.playerCount -= count;
	if (it->second.playerCount <= 0)
	{
		it->second.playerCount = 0;

		if (!it->second.pendingDestroy)
			it->second.pendingDestroy = true;
	}
}

void WorldService::CommitDestroy()
{
	for (auto it = _instances.begin(); it != _instances.end(); )
	{
		const WorldId id = it->first;
		Instance& inst = it->second;

		if (id == _squareWorldId)
		{
			inst.pendingDestroy = false;
			++it;
			continue;
		}

		if (inst.playerCount > 0)
		{
			inst.pendingDestroy = false;
			++it;
			continue;
		}

		if (inst.pendingDestroy)
		{
			if (_scheduler)
				_scheduler->Unregister(id);

#ifdef _DEBUG
		else
			throw std::runtime_error("스케줄러 초기화 누락");
#endif

			if (inst.instanceKey != 0)
			{
				InstKey key{ inst.type, inst.instanceKey };

				auto it2 = _resolved.find(key);
				if (it2 != _resolved.end() && it2->second == id)
					_resolved.erase(it2);
			}

			if (_reg.IsAlive(id))
				_reg.DestroyWorld(id);

			it = _instances.erase(it);
			continue;
		}

		++it;
	}
}

void WorldService::Clear()
{
	_squareWorldId = WorldId::Invalid();
	_resolved.clear();
	_instances.clear();
}

WorldId WorldService::CreateWorld(WorldType type, uint64 key)
{
	WorldDesc temp
	{
		.type = type,
		.worldRulesetId = 0,
		.Capacity = 3,
		.mapId = 0,
		.netPolicy = 0,
	};
	WorldId id = _reg.CreateWorld(temp);
	if (!id.IsValid()) return WorldId::Invalid();

	// TEMP : WorldDesc의 TickRate로 설정
	if (_scheduler)
		_scheduler->Register(id, 60);

#ifdef _DEBUG
	else
		throw std::runtime_error("스케줄러 초기화 누락");
#endif

	Instance inst
	{
		.type = type,
		.instanceKey = key,
		.playerCount = 0,
		.pendingDestroy = true,
	};
	auto [it, inserted] = _instances.try_emplace(id, inst);
	assert(inserted);
	return id;
}
