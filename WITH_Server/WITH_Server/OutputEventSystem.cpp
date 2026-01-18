#include "pch.h"
#include "OutputEventSystem.h"
#include "Framework.h"
#include "Math.h"
#include "NetHelper.h"

OutputEventSystem::OutputEventSystem(ECS& ecs, int p) : System(ecs, p)
{
	_handlers[DirtyType::Spawned] = [&](const OutputEvent& ev) { ProcessSpawn(ev); };
	_handlers[DirtyType::Despawned] = [&](const OutputEvent& ev) { ProcessDespawn(ev); };
	_handlers[DirtyType::Moved] = [&](const OutputEvent& ev) { ProcessMove(ev); };
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
	framework.listener.Send(sessionId, data);

	// 2. Spawn된 Player의 정보를 모든 Player에게 전송
	if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(event.entity))
	{
		float yaw = TransformHelper::QuaternionToYaw(trans->rotation);
		SendBuffer* data2 = NetHelper::SCAddPacket(sessionId,
			trans->position.x, trans->position.y, trans->position.z, yaw);
		framework.listener.Broadcast(data2);
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
			framework.listener.Send(sessionId, data3);
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
	int sessionId = it->second;

	// 1. Despawn된 Player의 정보를 모든 Player에게 전송
	SendBuffer* data = NetHelper::SCRemovePacket(sessionId);
	framework.listener.Broadcast(data);

	// 2. Despawn된 Player의 Session 정보를 EntityToSession 맵에서 제거
	ets.erase(it);

	// 3. Despawn된 Player의 Entity를 ECS에서 제거
	ecs.entityMng.Destroy(entity);

	// 4. Despawn된 Player의 Entity에 할당된 모든 컴포넌트 제거
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
	ecs.GetStorage<Collider>().RemoveComponent(entity);
	ecs.GetStorage<AttackState>().RemoveComponent(entity);
	ecs.GetStorage<ParryBuf>().RemoveComponent(entity);
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
		framework.listener.Broadcast(data);

//#ifdef _DEBUG
//		std::cout << "[OutputEventSystem] (" << trans->position.x << ", "
//			<< trans->position.y << ", " << trans->position.z << ")\n";
//#endif

	}
}
