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
	virtual ~NetIdMap() = default;

	void OnConnected(uint32 connId);
	void OnDisconnected(uint32 connId);
	
	void EnterWorld(uint32 connId, WorldId world);
	void LeaveWorld(uint32 connId);

	void BindPlayer(uint32 connId, NetId player);

	uint32 GetConn(NetId player) const;
	NetId GetPlayer(uint32 connId) const;
	WorldId GetWorld(NetId player) const;

private:
	std::unordered_map<uint32, NetBinding> _bindMap;
	std::unordered_map<NetId, uint32> _connByNetId;
};

