#include "pch.h"
#include "WorldLeapManager.h"
#include "WorldLeapData.h"
#include "WorldService.h"
#include "WorldRuntime.h"

void WorldLeapManager::Request(const WorldLeapRequest& request)
{
	_requests.push(request);
}

void WorldLeapManager::CommitFrame()
{
	WorldLeapRequest request;
	while (_requests.try_pop(request))
	{
		if (request.connIds.empty()) continue;

		switch (request.mode) {
		case LeapMode::Default:			HandleDefault(request);			break;
		case LeapMode::RecoverToSquare: HandleRecoverToSquare(request); break;
		}
	}
}

void WorldLeapManager::HandleDefault(const WorldLeapRequest& request)
{
	const WorldId fromWorldId = request.fromWorldId;

	World* fromWorld = static_cast<World*>(_reg.GetWorld(fromWorldId));
	if (!fromWorld)
	{
		// TODO : StaleFromWorld
		return;
	}

	const WorldId toWorldId = _service.ResolveTargetWorld(request.toWorldType, request.instanceKey);
	if (!toWorldId.IsValid())
	{
		// TODO : WorldCreateFail
		return;
	}

	if (fromWorldId == toWorldId) return;

	World* toWorld = static_cast<World*>(_reg.GetWorld(toWorldId));
	if (!toWorld)
	{
		// TODO : ToWorldDead
		return;
	}

	WorldRuntime& fromRt = fromWorld->Runtime();
	WorldRuntime& toRt = toWorld->Runtime();

	struct SnapShot { uint32 connId; PlayerSnapshot snap; };
	std::vector<SnapShot> snapshots;
	snapshots.reserve(request.connIds.size());

	// 1. SnapShot 생성
	bool snapOk{ true };
	for (uint32 connId : request.connIds)
	{
		PlayerSnapshot snap;
		if (!fromWorld->TryMakeSnapshot(fromRt, connId, snap))
		{
			snapOk = false;
			break;
		}
		snapshots.push_back(SnapShot{ connId, snap });
	}

	if (!snapOk)
	{
		// TODO : SnapshotFail
		return;
	}

	// 2. Snapshot 기반으로 Spawn/Despawn + Apply
	std::vector<uint32> spawned;
	spawned.reserve(snapshots.size());

	bool spawnOk{ true };
	bool applyOk{ true };
	for (const SnapShot& snapshot : snapshots)
	{
		if (toWorld->SpawnPlayer(snapshot.connId).IsNull())
		{
			spawnOk = false;
			break;
		}
		spawned.push_back(snapshot.connId);

		if (!toWorld->ApplySnapshot(toRt, snapshot.connId, snapshot.snap))
		{
			applyOk = false;
			break;
		}
	}

	if (!spawnOk)
	{
		// TODO : SpawnFail
		for (uint32 connId : spawned)
			toWorld->DespawnPlayer(toRt, connId);

		WorldLeapRequest leap
		{
			.connIds = request.connIds,
			.fromWorldId = WorldId::Invalid(),
			.toWorldType = WorldType::Square,
			.instanceKey = 0,
			.mode = LeapMode::RecoverToSquare
		};
		_requests.push(leap);
		return;
	}

	if (!applyOk)
	{
		// TODO : ApplyFail
		for (uint32 connId : spawned)
			toWorld->DespawnPlayer(toRt, connId);

		WorldLeapRequest leap
		{
			.connIds = request.connIds,
			.fromWorldId = WorldId::Invalid(),
			.toWorldType = WorldType::Square,
			.instanceKey = 0,
			.mode = LeapMode::RecoverToSquare
		};
		_requests.push(leap);
		return;
	}

	for (const SnapShot& snapshot : snapshots)
		fromWorld->DespawnPlayer(fromRt, snapshot.connId);

	const uint32 playerCnt = static_cast<uint32>(snapshots.size());

	_service.OnPlayerLeave(fromWorldId, playerCnt);
	_service.OnPlayerEnter(toWorldId, playerCnt);
}

void WorldLeapManager::HandleRecoverToSquare(const WorldLeapRequest& request)
{
	const WorldId squareId = _service.ResolveTargetWorld(WorldType::Square, 0);
	if (!squareId.IsValid())
	{
		// TODO : 복구 불가
		return;
	}

	World* squareWorld = static_cast<World*>(_reg.GetWorld(squareId));
	if (!squareWorld)
	{
		// TODO : 복구 불가
		return;
	}

	[[maybe_unused]] WorldRuntime& squareRt = squareWorld->Runtime();

	uint32 spawnedCount{ 0 };
	for (uint32 connId : request.connIds)
	{
		if (squareWorld->HasPlayer(connId)) continue;

		Entity e = squareWorld->SpawnPlayer(connId);
		if (e.IsNull())
		{
			// TODO : 복구 불가
			continue;
		}

		{
			// TODO : Player 기본 초기화
		}
		spawnedCount++;
	}

	if(spawnedCount > 0)
		_service.OnPlayerEnter(squareId, spawnedCount);
}
