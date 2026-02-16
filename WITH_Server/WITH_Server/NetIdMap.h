#pragma once

#include "NetId.h"
#include "WorldId.h"

struct NetBinding
{
	WorldId world;
	NetId player;
	bool inWorld;
};

class NetIdMap {
public:
	void OnConnected(uint32 connId);
	void OnDisconnected(uint32 connId);
	
	void EnterWorld(uint32 connId, WorldId world);
	void LeaveWorld(uint32 connId);

	void BindPlayer(uint32 connId, NetId player);

	NetId GetPlayer(uint32 connId) const;
	WorldId GetWorld(NetId player) const;

private:
	std::unordered_map<uint32, NetBinding> _bindMap;
};

