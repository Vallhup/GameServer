#include "pch.h"
#include "OutputEventSystem.h"
#include "Framework.h"
#include "Math.h"
#include "NetHelper.h"
#include "RepComponent.h"

OutputEventSystem::OutputEventSystem(WorldRuntime& rt, int p) : System(rt, p)
{
	_outBuffers.resize(5000);

	_handlers[DirtyType::Spawned] = [&](const OutputEvent& ev) { ProcessSpawn(ev); };
	_handlers[DirtyType::Despawned] = [&](const OutputEvent& ev) { ProcessDespawn(ev); };
	_handlers[DirtyType::Moved] = [&](const OutputEvent& ev) { ProcessMove(ev); };
	_handlers[DirtyType::AnimationChanged] = [&](const OutputEvent& ev) { ProcessAnimationChange(ev); };
}

void OutputEventSystem::Execute(const double dT)
{
	OutputEvent ev;
	while (Framework::Get().outEventQueue.try_pop(ev))
	{
		auto it = _handlers.find(ev.type);
		if (it != _handlers.end())
			it->second(ev);

#ifdef _DEBUG
		else
			std::cout << "[OutputEventSystem] Unknown Type Event\n";
#endif
	}

	FlushAll();
}

void OutputEventSystem::ProcessSpawn(const OutputEvent& event)
{
	ECS& ecs = _runtime.GetECS();
 	auto& framework = Framework::Get();

	NetId myId = event.netId;
	Entity myEntity = framework.netIdRegistry.FindEntity(myId);
	uint32 myConnId = framework.listener.GetIdMap().GetConn(myId);

	// 1. Spawn된 Player에게 자신의 Login 정보 전송
	SendBuffer* data = NetHelper::SCLoginPacket(myId);
	EnqueueToSession(myConnId, data->data, data->size);
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
			EnqueueToSession(connId, data2->data, data2->size);
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
			EnqueueToSession(myConnId, data3->data, data3->size);
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
		SendBuffer* data = NetHelper::SCAddPacket(netComp->id, spawnComp->type,
			trans->position.x, trans->position.y, trans->position.z, yaw);

		EnqueueToSession(myConnId, data->data, data->size);
		SendBufferPool::Get().Release(data);
	}
}

void OutputEventSystem::ProcessDespawn(const OutputEvent& event)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();
	
	NetId id = event.netId;
	Entity entity = framework.netIdRegistry.FindEntity(id);
	framework.netIdRegistry.UnbindEntity(id);

	// 1. Despawn된 Player의 정보를 모든 Player에게 전송
	SendBuffer* data = NetHelper::SCRemovePacket(id);
	
	std::vector<uint32> connIds;
	framework.listener.GetConnRegistry().FillConnIds(connIds);

	for (uint32 connId : connIds)
	{
		EnqueueToSession(connId, data->data, data->size);
	}

	SendBufferPool::Get().Release(data);

	// 2. Despawn된 Player의 Entity에 할당된 모든 컴포넌트 제거
	// 4. Despawn된 Player의 Entity를 ECS에서 제거
	ecs.DestroyEntity(entity);
	
	// 5. Despawn된 Player의 Session 정보를 outBuffers 맵에서 제거
	uint32 connId = framework.listener.GetIdMap().GetConn(id);

	SendBufferPool::Get().Release(_outBuffers[connId]);
	_outBuffers[connId] = nullptr;

	framework.listener.GetIdMap().OnDisconnected(connId);
}

void OutputEventSystem::ProcessMove(const OutputEvent& event)
{
	auto& framework = Framework::Get();

	NetId myId = event.netId;
	Entity myEntity = framework.netIdRegistry.FindEntity(myId);
	uint32 myConnId = framework.listener.GetIdMap().GetConn(myId);

	if (const auto* trans = _runtime.GetECS().GetStorage<Transform>().GetComponent(myEntity))
	{
		float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
		SendBuffer* data = NetHelper::SCMovePacket(myId,
			trans->position.x, trans->position.y, trans->position.z, yaw);

		std::vector<uint32> connIds;
		framework.listener.GetConnRegistry().FillConnIds(connIds);

		for (uint32 connId : connIds)
		{
			EnqueueToSession(connId, data->data, data->size);
		}

		SendBufferPool::Get().Release(data);
	}
}

void OutputEventSystem::ProcessAnimationChange(const OutputEvent& event)
{
	auto& framework = Framework::Get();

	NetId myId = event.netId;
	Entity myEntity = framework.netIdRegistry.FindEntity(myId);
	uint32 myConnId = framework.listener.GetIdMap().GetConn(myId);

	SendBuffer* data = NetHelper::SCAnimationChangePacket(myId, event.payload.anim.currType);

	std::vector<uint32> connIds;
	framework.listener.GetConnRegistry().FillConnIds(connIds);

	for (uint32 connId : connIds)
	{
		EnqueueToSession(connId, data->data, data->size);
	}

	SendBufferPool::Get().Release(data);
}

void OutputEventSystem::EnqueueToSession(uint32 sid, const void* data, 
	uint32 len)
{
	auto& buffer = _outBuffers[sid];

	if (!buffer)
	{
		buffer = SendBufferPool::Get().
			Acquire(std::max<uint32>(len, 4096));
	}

	if (!buffer->TryAppend(data, len))
	{
		FlushOne(sid, buffer);

		buffer = SendBufferPool::Get().
			Acquire(std::max<uint32>(len, 4096));
		buffer->TryAppend(data, len);
	}
}

void OutputEventSystem::FlushOne(uint32 sid, SendBuffer*& buffer)
{
	if (!buffer || buffer->size == 0) return;
	Framework::Get().listener.Send(sid, buffer);
	buffer = nullptr;
}

void OutputEventSystem::FlushAll()
{
	for (uint32 i = 0; i < (uint32)_outBuffers.size(); ++i)
	{
		if (!_outBuffers[i]) continue;
		FlushOne(i, _outBuffers[i]);
	}
}
