#include "OutputEventSystem.h"
#include "Framework.h"

#include "Protocol.hpp"
#include "Protocols/Protocol.pb.h"

#include "Math.h"

OutputEventSystem::OutputEventSystem(ECS& ecs, int p) : System(ecs, p)
{
	_handlers[DirtyType::Spawned] = [&](const OutputEvent& ev) { ProcessSpawn(ev); };
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
	int sessionId = it->second;

	// 1. Spawn된 Player에게 자신의 Login 정보 전송
	SendBuffer data = PacketFactory::SCLoginPacket(sessionId);
	framework.network.Send(sessionId, data.data());

	// 2. Spawn된 Player의 정보를 모든 Player에게 전송
	if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(event.entity))
	{
		SendBuffer data2 = PacketFactory::SCAddPacket(sessionId, 
			trans->position.x, trans->position.y, trans->position.z);
		framework.network.Broadcast(data2.data());
	}
	
	// 3. 기존 Player들의 정보를 Spawn된 Player에게 전송
	for (const auto& [entity, sessId] : ets)
	{
		if (sessId == sessionId) continue;
		if (const auto* trans = ecs.GetStorage<Transform>().GetComponent(entity))
		{
			SendBuffer data3 = PacketFactory::SCAddPacket(
				sessId, trans->position.x, trans->position.y, trans->position.z);
			framework.network.Send(sessionId, data3.data());
		}
	}
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
		SendBuffer data = PacketFactory::SCMovePacket(sessionId, 
			trans->position.x, trans->position.y, trans->position.z, yaw);
		framework.network.Broadcast(data.data());
	}
}
