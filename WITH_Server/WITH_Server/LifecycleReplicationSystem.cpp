#include "pch.h"
#include "LifecycleReplicationSystem.h"
#include "Event.h"
#include "Framework.h"
#include "Tags.h"

void LifecycleReplicationSystem::Execute(const double dT)
{
	Framework& framework = Framework::Get();

	LifecycleEvent ev;
	while (framework.lifecycleEventQueue.try_pop(ev))
	{
		switch (ev.type) {
		case LifecycleEventType::Spawned:
		{
			ProcessSpawn(ev);
			break;
		}
		case LifecycleEventType::Despawned:
		{
			ProcessDespawn(ev);
			break;
		}
#ifdef _DEBUG
		default:
		{
			std::cout << "[OutputEventSystem] Unknown Type Event\n";
		}
#endif
		}
	}

	framework.listener.SendBuffers().FlushAll();
}

void LifecycleReplicationSystem::ProcessSpawn(const LifecycleEvent& event)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();
	SessionSendBufferManager& bufferMng = framework.listener.SendBuffers();

	NetId myId = event.netId;
	Entity myEntity = framework.netIdRegistry.FindEntity(myId);
	uint32 myConnId = framework.listener.GetIdMap().GetConn(myId);
	assert(myConnId != std::numeric_limits<uint32_t>::max());

	// 1. Spawn된 Player에게 자신의 Login 정보 전송
	SendBuffer* data = NetHelper::SCLoginPacket(myId);
	bufferMng.Enqueue(myConnId, data->data, data->size);
	SendBufferPool::Get().Release(data);

	std::vector<uint32> connIds;
	framework.listener.GetConnRegistry().FillConnIds(connIds);

	// 2. Spawn된 Player의 정보를 모든 Player에게 전송
	if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(myEntity))
	{
		const auto* typeComp = ecs.GetStorage<SpawnTypeComp>().GetComponent(myEntity);
		if (!typeComp) return;

		EntityType type = typeComp->type;
		float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
		SendBuffer* data2 = NetHelper::SCAddPacket(myId, type,
			trans->position.x, trans->position.y, trans->position.z, yaw);

		for (uint32 connId : connIds)
		{
			bufferMng.Enqueue(connId, data2->data, data2->size);
		}

		SendBufferPool::Get().Release(data2);
	}

	// 3. 기존 Player들의 정보를 Spawn된 Player에게 전송
	for (uint32 connId : connIds)
	{
		if (connId == myConnId) continue;

		NetId id = framework.listener.GetIdMap().GetPlayer(connId);
		Entity entity = framework.netIdRegistry.FindEntity(id);
		if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(entity))
		{
			const auto* typeComp = ecs.GetStorage<SpawnTypeComp>().GetComponent(entity);
			if (!typeComp) continue;

			EntityType type = typeComp->type;
			float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
			SendBuffer* data3 = NetHelper::SCAddPacket(id, type,
				trans->position.x, trans->position.y, trans->position.z, yaw);

			bufferMng.Enqueue(myConnId, data3->data, data3->size);
			SendBufferPool::Get().Release(data3);
		}
	}

	// 4. Monster의 정보를 Spawn된 Player에게 전송
	for (const Entity& e : ecs.AliveEntities())
	{
		const auto* trans = ecs.GetStorage<Transform>().GetComponent(e);
		const auto* spawnComp = ecs.GetStorage<SpawnTypeComp>().GetComponent(e);
		const auto* netComp = ecs.GetStorage<NetIdComp>().GetComponent(e);
		if (!trans || !spawnComp || !netComp) continue;

		if (ecs.GetStorage<DisconnectedTag>().HasComponent(e)) continue;
		if (ecs.GetStorage<PlayerTag>().HasComponent(e)) continue;

		float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
		SendBuffer* data4 = NetHelper::SCAddPacket(netComp->id, spawnComp->type,
			trans->position.x, trans->position.y, trans->position.z, yaw);

		bufferMng.Enqueue(myConnId, data4->data, data4->size);
		SendBufferPool::Get().Release(data4);
	}
}

void LifecycleReplicationSystem::ProcessDespawn(const LifecycleEvent& event)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();

	NetId myNetId = event.netId;
	uint32_t myConnId = event.connId;
	Entity myEntity = event.entity;

	framework.netIdRegistry.UnbindEntity(myNetId);

	SendBuffer* data = NetHelper::SCRemovePacket(myNetId);

	std::vector<uint32_t> connIds;
	framework.listener.GetConnRegistry().FillConnIds(connIds);

	for (uint32_t connId : connIds)
	{
		framework.listener.SendBuffers().
			Enqueue(connId, data->data, data->size);
	}
	SendBufferPool::Get().Release(data);

	_runtime.DeferredDestroyEntity(myEntity);
}
