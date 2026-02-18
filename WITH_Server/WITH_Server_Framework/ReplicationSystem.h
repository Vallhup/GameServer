#pragma once

#include "SessionRepState.h"
#include "DirtyTracker.h"
#include "ServerContext.h"

class ConnectionRegistry;

// 틱마다 프레임 생성 / 전송

class ReplicationSystem {
public:
	ReplicationSystem(ServerContext& ctx, WorldRuntime& rt);

	void OnSessionEnter(uint32 sessId);
	void OnSessionLeave(uint32 sessId);

	void NotifySpawn(Entity e);
	void NotifyDespawn(Entity e);
	void MarkDirty(uint64 id, uint32 mask);

	void Execute(uint32 serverTick);

private:
	void BuildSpawns(uint32 sessId, SessionRepState& state, Protocol::SC_REPLICATION_FRAME_PACKET& frame);
	void BuildUpdates();
	void BuildDespawns();
	void SendFrame();

	static bool TryFindConnIdByOwnerNetId(
		const IConnContext& connCtx,
		const ConnectionRegistry& conns,
		NetId owner, uint32& outConnId);

	WorldRuntime& _runtime;

	IConnContext& _connCtx;
	NetIdRegistry& _netIds;
	//ConnectionRegistry& _connRegistry;

	uint32 _frameSeq;
	std::unordered_map<uint32, SessionRepState> _sessions;

	std::vector<Entity> _spawned;
	std::vector<Entity> _despawned;

	DirtyTracker _tracker;
};