#include "pch.h"
#include "OutputEventSystem.h"
#include "Framework.h"
#include "Math.h"
#include "NetHelper.h"

OutputEventSystem::OutputEventSystem(ECS& ecs, int p) : System(ecs, p)
{
	_outBuffers.resize(5000);

	_handlers[DirtyType::Spawned] = [&](const OutputEvent& ev) { ProcessSpawn(ev); };
	_handlers[DirtyType::Despawned] = [&](const OutputEvent& ev) { ProcessDespawn(ev); };
	_handlers[DirtyType::Moved] = [&](const OutputEvent& ev) { ProcessMove(ev); };
	_handlers[DirtyType::AnimationChanged] = [&](const OutputEvent& ev) { ProcessAnimationChange(ev); };
}

void OutputEventSystem::Execute(const float dT)
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
 	auto& framework = Framework::Get();
	auto& ets = framework.entityToSession;

	auto it = ets.find(event.entity);
	if (it == ets.end()) return;
	uint32 sessionId = it->second;

	// 1. Spawn된 Player에게 자신의 Login 정보 전송
	SendBuffer* data = NetHelper::SCLoginPacket(sessionId);
	EnqueueToSession(sessionId, data->data, data->size);
	SendBufferPool::Get().Release(data);

	// 2. Spawn된 Player의 정보를 모든 Player에게 전송
	if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(event.entity))
	{
		float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
		SendBuffer* data2 = NetHelper::SCAddPacket(sessionId,
			trans->position.x, trans->position.y, trans->position.z, yaw);
		for (const auto& [entity, sid] : ets)
		{
			EnqueueToSession(sid, data2->data, data2->size);
		}

		SendBufferPool::Get().Release(data2);
	}
	
	// 3. 기존 Player들의 정보를 Spawn된 Player에게 전송
	for (const auto& [entity, sessId] : ets)
	{
		if (sessId == sessionId) continue;
		if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(entity))
		{
			float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
			SendBuffer* data3 = NetHelper::SCAddPacket(
				sessId, trans->position.x, trans->position.y, trans->position.z, yaw);
			EnqueueToSession(sessionId, data3->data, data3->size);
			SendBufferPool::Get().Release(data3);
		}
	}
}

void OutputEventSystem::ProcessDespawn(const OutputEvent& event)
{
	Entity entity = event.entity;

	auto& framework = Framework::Get();
	auto& ets = framework.entityToSession;

	auto it = ets.find(event.entity);
	if (it == ets.end()) return;
	uint32 sessionId = it->second;

	// 1. Despawn된 Player의 정보를 모든 Player에게 전송
	SendBuffer* data = NetHelper::SCRemovePacket(sessionId);
	for (const auto& [entity, sid] : ets)
	{
		EnqueueToSession(sid, data->data, data->size);
	}

	SendBufferPool::Get().Release(data);

	// 2. Despawn된 Player의 Entity에 할당된 모든 컴포넌트 제거
	ecs.GetStorage<Transform>().RemoveComponent(entity);
	ecs.GetStorage<Velocity>().RemoveComponent(entity);
	ecs.GetStorage<ActionMoveDelta>().RemoveComponent(entity);
	ecs.GetStorage<LocomotionMoveDelta>().RemoveComponent(entity);
	ecs.GetStorage<LocomotionAnimPhase>().RemoveComponent(entity);
	ecs.GetStorage<LocomotionState>().RemoveComponent(entity);
	ecs.GetStorage<ActionIntent>().RemoveComponent(entity);
	ecs.GetStorage<ActionState>().RemoveComponent(entity);
	ecs.GetStorage<AttackData>().RemoveComponent(entity);
	ecs.GetStorage<Health>().RemoveComponent(entity);
	ecs.GetStorage<AnimationState>().RemoveComponent(entity);
	ecs.GetStorage<Animator>().RemoveComponent(entity);
	ecs.GetStorage<CombatCollider>().RemoveComponent(entity);
	ecs.GetStorage<AttackState>().RemoveComponent(entity);
	ecs.GetStorage<ParryBuf>().RemoveComponent(entity);
	ecs.GetStorage<DisconnectedTag>().RemoveComponent(entity);
	ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);

	// 3. Despawn된 Player의 Session 정보를 EntityToSession 맵에서 제거
	// 4. Despawn된 Entity의 정보를 SessionToEntity 맵에서 제거
	// 4. Despawn된 Player의 Entity를 ECS에서 제거
	ecs.entityMng.DestoryPlayer(entity, sessionId);
	
	// 5. Despawn된 Player의 Session 정보를 outBuffers 맵에서 제거
	SendBufferPool::Get().Release(_outBuffers[sessionId]);
	_outBuffers[sessionId] = nullptr;
}

void OutputEventSystem::ProcessMove(const OutputEvent& event)
{
	auto& framework = Framework::Get();
	auto& ets = framework.entityToSession;

	auto it = ets.find(event.entity);
	if (it == ets.end()) return;
	int sessionId = it->second;

	if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(event.entity))
	{
		float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
		SendBuffer* data = NetHelper::SCMovePacket(sessionId,
			trans->position.x, trans->position.y, trans->position.z, yaw);
		for (const auto& [entity, sid] : ets)
		{
			EnqueueToSession(sid, data->data, data->size);
		}

		SendBufferPool::Get().Release(data);
	}
}

void OutputEventSystem::ProcessAnimationChange(const OutputEvent& event)
{
	auto& framework = Framework::Get();
	auto& ets = framework.entityToSession;

	auto it = ets.find(event.entity);
	if (it == ets.end()) return;
	int sessionId = it->second;

	SendBuffer* data = NetHelper::SCAnimationChangePacket(
		sessionId, event.payload.anim.currType);
	for (const auto& [entity, sid] : ets)
	{
		EnqueueToSession(sid, data->data, data->size);
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
	for (uint32 i = 0; i < _outBuffers.size(); ++i)
	{
		if (!_outBuffers[i]) continue;
		FlushOne(i, _outBuffers[i]);
	}
}
