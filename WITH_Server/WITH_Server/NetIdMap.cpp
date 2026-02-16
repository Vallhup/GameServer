#include "pch.h"
#include "NetIdMap.h"

void NetIdMap::OnConnected(uint32 connId)
{
	_bindMap.try_emplace(connId,
		WorldId::Invalid(), NetId::Invalid(), false);
}

void NetIdMap::OnDisconnected(uint32 connId)
{
	_bindMap.erase(connId);
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
	_bindMap.at(connId).player = player;
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
