#include "pch.h"
#include "EventSystem.h"

#include "Framework.h"
#include "Entity.h"
#include "Component.h"
#include "RepComponent.h"

#include "Intent.h"
#include "Tags.h"
#include "Movement.h"

EventSystem::EventSystem(WorldRuntime& rt, int p) : System(rt, p) 
{
	_handlers[EventType::EV_CONNECT] = [&](const Event& ev) { ProcessConnect(ev); };
	_handlers[EventType::EV_DISCONNECT] = [&](const Event& ev) { ProcessDisconnect(ev); };
	_handlers[EventType::EV_MOVE] = [&](const Event& ev) { ProcessMove(ev); };
	_handlers[EventType::EV_ACTION] = [&](const Event& ev) { ProcessAction(ev); };
}

void EventSystem::Execute(const double dT)
{
	Event ev;
	while (Framework::Get().eventQueue.try_pop(ev))
	{
		auto it = _handlers.find(ev.type);
		if (it != _handlers.end())
			it->second(ev);

#ifdef _DEBUG
		else
			std::cout << "[EventSystem] Unknown Type Event\n";
#endif
	}
}

void EventSystem::ProcessConnect(const Event& event)
{
	const auto* p = std::get_if<ConnectEvent>(&event.payload);
	if (!p) return;

#ifdef _DEBUG
	std::cout << "[EventSystem] Player[" << p->sessionId << "] Login\n";
#endif

	Framework& framework = Framework::Get();

	const uint32 connId = p->sessionId;
	Entity entity = _runtime.SpawnPlayer(connId);
	NetId nId = framework.listener.GetIdMap().GetPlayer(connId);

	framework.outEventQueue.push(OutputEvent{ nId, DirtyType::Spawned });
}

void EventSystem::ProcessDisconnect(const Event& event)
{
	const auto* p = std::get_if<DisconnectEvent>(&event.payload);

#ifdef _DEBUG
	std::cout << "[EventSystem] Player[" << p->id.GetId() << "] Disconnect\n";
#endif
	Framework& framework = Framework::Get();
	Entity entity = framework.netIdRegistry.FindEntity(p->id);

	if(!entity.IsNull())
	{
		_runtime.GetECS().GetStorage<DisconnectedTag>().AddComponent(entity);
		framework.outEventQueue.push(OutputEvent{ p->id, DirtyType::Despawned });
	}
}

void EventSystem::ProcessMove(const Event& event)
{
	const auto* p = std::get_if<MoveEvent>(&event.payload);
	if (!p) return;

	Framework& framework = Framework::Get();
	Entity entity = framework.netIdRegistry.FindEntity(p->id);

	if (auto* velocity = _runtime.GetECS().GetStorage<Velocity>().GetComponent(entity))
	{
		if (auto* loco = _runtime.GetECS().GetStorage<LocomotionState>().GetComponent(entity))
		{
			int inputX = p->inputX;
			int inputZ = p->inputZ;
			double yaw = p->yaw;
			bool isRun = p->isRun;

			XMVECTOR forward = XMVectorSet(sin(yaw), 0, cos(yaw), 0);
			XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward);

			XMVECTOR dir = XMVectorAdd(
				XMVectorScale(forward, inputZ),
				XMVectorScale(right, inputX)
			);

			if (p->inputX == 0 && p->inputZ == 0)
			{
				loco->isMoving = false;
				loco->isRun = false;
				dir = XMVectorZero();

				XMStoreFloat3(&velocity->dir, dir);
			}

			else
			{
				loco->isMoving = true;
				loco->isRun = isRun;
				dir = XMVector3Normalize(dir);

				XMStoreFloat3(&velocity->dir, dir);
			}
		}
	}
}

void EventSystem::ProcessAction(const Event& event)
{
	const auto* p = std::get_if<ActionEvent>(&event.payload);
	if (!p) return;

	Framework& framework = Framework::Get();
	Entity entity = framework.netIdRegistry.FindEntity(p->id);

	if (auto* actionIntent = _runtime.GetECS().GetStorage<ActionIntent>().GetComponent(entity))
	{
		switch (p->type) {
		case ActionRequestType::Attack:
		{
			ActionRequestEvent ev
			{
				.entity = entity,
				.actionType = ActionType::Attack,
				.attackType = AttackType::Light,
				.reason = ActionRequestReason::FromInput
			};

			_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
			break;
		}
		case ActionRequestType::Dodge:
		{
			ActionRequestEvent ev
			{
				.entity = entity,
				.actionType = ActionType::Dodge,
				.attackType = AttackType::None,
				.reason = ActionRequestReason::FromInput
			};

			_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
			break;
		}
		case ActionRequestType::Parry:
		{
			ActionRequestEvent ev
			{
				.entity = entity,
				.actionType = ActionType::Parry,
				.attackType = AttackType::None,
				.reason = ActionRequestReason::FromInput
			};

			_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
			break;
		}
		case ActionRequestType::Guard:
		{
			actionIntent->guard = p->input;
			break;
		}
		}
	}
}
