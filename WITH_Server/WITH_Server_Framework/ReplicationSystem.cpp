#include "pch.h"
#include "ReplicationSystem.h"
#include "IConnContext.h"
#include "ConnectionRegistry.h"
#include "RepComponent.h"

ReplicationSystem::ReplicationSystem(ServerContext& ctx, WorldRuntime& rt)
	: _runtime(rt), _connCtx(*ctx.connContext), _netIds(ctx.netIdRegistry), _frameSeq(0)
{
}

void ReplicationSystem::OnSessionEnter(uint32 sessId)
{
	auto& state = _sessions[sessId];
	state.sessId = sessId;
	state.knownIds.clear();

	// TODO : Login Packet Send
}

void ReplicationSystem::OnSessionLeave(uint32 sessId)
{
	_sessions.erase(sessId);
}

void ReplicationSystem::NotifySpawn(Entity e)
{
	_spawned.push_back(e);
}

void ReplicationSystem::NotifyDespawn(Entity e)
{
	_despawned.push_back(e);
}

void ReplicationSystem::MarkDirty(uint64 id, uint32 mask)
{
	_tracker.Mark(id, mask);
}

void ReplicationSystem::Execute(uint32 serverTick)
{
	++_frameSeq;

	for (auto& [sessId, state] : _sessions)
	{
		Protocol::SC_REPLICATION_FRAME_PACKET frame;
		frame.set_frameseq(_frameSeq);
		frame.set_servertick(serverTick);

		BuildSpawns(sessId, state, frame);
		BuildUpdates();
		BuildDespawns();

		SendFrame();
	}

	_spawned.clear();
	_despawned.clear();
	_tracker.Clear();
}

void ReplicationSystem::BuildSpawns(uint32 sessId, SessionRepState& state, Protocol::SC_REPLICATION_FRAME_PACKET& frame)
{
	//ECS& ecs = _runtime.GetECS();

	//// 해당 월드의 모든 엔티티에서 아직 해당 세션이 모르는 엔티티면 spawn
	//WorldId world;
	//if (!_connCtx.TryGetWorld(sessId, world))
	//	return;

	//for (const auto& [entity, netComp, worldComp, spawnType] : ecs.View<NetIdComp, WorldIdComp, SpawnTypeComp>())
	//{
	//	if (worldComp.id != world) continue;
	//	if (!netComp.id.IsValid()) continue;


	//}
}

void ReplicationSystem::BuildUpdates()
{
}

void ReplicationSystem::BuildDespawns()
{
}

void ReplicationSystem::SendFrame()
{
}

bool ReplicationSystem::TryFindConnIdByOwnerNetId(const IConnContext& connCtx, const ConnectionRegistry& conns, NetId owner, uint32& outConnId)
{
	std::vector<uint32> connIdList;
	conns.FillConnIds(connIdList);

	for (uint32 connId : connIdList)
	{
		NetId nId;
		if (!connCtx.TryGetOwnerPlayer(connId, nId))
			continue;

		if (nId == owner)
		{
			outConnId = connId;
			return true;
		}
	}

	return false;
}
