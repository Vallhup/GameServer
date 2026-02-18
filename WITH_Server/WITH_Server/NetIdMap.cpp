#include "pch.h"
#include "NetIdMap.h"

bool NetIdMap::TryGetWorld(uint32 connId, WorldId& out) const 
{
	auto it = _bindMap.find(connId);
	if (it == _bindMap.end()) return false;

	out = it->second.world;
	return true;
}

bool NetIdMap::TryGetOwnerPlayer(uint32 connId, NetId& out) const
{
	auto it = _bindMap.find(connId);
	if (it == _bindMap.end()) return false;

	out = it->second.player;
	return true;
}

void NetIdMap::OnConnected(uint32 connId)
{
	_bindMap.try_emplace(connId,
		WorldId::Invalid(), NetId::Invalid(), false);
}

void NetIdMap::OnDisconnected(uint32 connId)
{
	auto it = _bindMap.find(connId);
	if (it == _bindMap.end()) return;

	const NetBinding& bind = it->second;
	if (bind.player.IsValid())
		_connByNetId.erase(bind.player);

	_bindMap.erase(it);
}

void NetIdMap::EnterWorld(uint32 connId, WorldId world)
{
	auto& bind = _bindMap.at(connId);
	bind.world = world;
	bind.inWorld = true;
}

void NetIdMap::LeaveWorld(uint32 connId)
{
	auto& bind = _bindMap.at(connId);
	bind.world = WorldId::Invalid();
	bind.inWorld = false;
}

void NetIdMap::BindPlayer(uint32 connId, NetId player)
{
	auto& bind = _bindMap[connId];

	if (bind.player.IsValid())
		_connByNetId.erase(bind.player);

	bind.player = player;

	if (player.IsValid())
	{
		auto [it, inserted] = _connByNetId.try_emplace(player, connId);
		assert(inserted && "connByNetId is not emplace.");

		auto it2 = _connByNetId.find(player);
		assert(it2 != _connByNetId.end());
		assert(it2->second == connId);
	}
}

uint32 NetIdMap::GetConn(NetId player) const
{
	auto it = _connByNetId.find(player);
	if (it == _connByNetId.end()) return std::numeric_limits<uint32>::max();
	return it->second;
}

NetId NetIdMap::GetPlayer(uint32 connId) const
{
	auto it = _bindMap.find(connId);
	if (it != _bindMap.end()) return it->second.player;
	return NetId::Invalid();
}

WorldId NetIdMap::GetWorld(NetId player) const
{
	auto it = _bindMap.find(player.GetId());
	if (it != _bindMap.end()) return it->second.world;
	return WorldId::Invalid();
}
