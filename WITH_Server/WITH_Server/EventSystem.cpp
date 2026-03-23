#include "pch.h"
#include "EventSystem.h"

#include "Framework.h"
#include "Entity.h"
#include "Component.h"
#include "RepComponent.h"

#include "Tags.h"
#include "Intent.h"
#include "Command.h"
#include "Movement.h"

EventSystem::EventSystem(WorldRuntime& rt) : System(rt) 
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
	std::cout << "[EventSystem] Player[" << p->connId << "] Login\n";
#endif

	Framework& framework = Framework::Get();

	const uint32 connId = p->connId;
	const Entity entity = _runtime.SpawnPlayer(connId);
	const NetId nId = framework.listener.GetIdMap().GetPlayer(connId);

	framework.lifecycleEventQueue.push(LifecycleEvent::Spawned(nId));
}

void EventSystem::ProcessDisconnect(const Event& event)
{
	const auto* p = std::get_if<DisconnectEvent>(&event.payload);
	if (!p) return;

#ifdef _DEBUG
	std::cout << "[EventSystem] Player[" << p->netId.GetId() << "] Disconnect\n";
#endif
	Framework& framework = Framework::Get();

	if(!p->entity.IsNull())
	{
		_runtime.ImmediateAddComponent<DisconnectedTag>(p->entity);
		framework.lifecycleEventQueue.push(
			LifecycleEvent::Despawned(p->netId, p->connId, p->entity));
	}
}

void EventSystem::ProcessMove(const Event& event)
{
	const auto* p = std::get_if<MoveEvent>(&event.payload);
	if (!p) return;

	Framework& framework = Framework::Get();
	const Entity entity = framework.netIdRegistry.FindEntity(p->id);
	if (entity.IsNull()) return;

	auto* cmd =
		_runtime.GetECS().GetStorage<EntityCommandFrame>().GetComponent(entity);
	if (!cmd) return;

	cmd->source = CommandSource::Player;

	if (p->inputX == 0 && p->inputZ == 0)
	{
		cmd->move.Clear();
		return;
	}

	cmd->move.hasMove = true;
	cmd->move.moveRun = p->isRun;
	cmd->move.moveDir = BuildMoveDirFromYawInput(p->inputX, p->inputZ, p->yaw);
}

void EventSystem::ProcessAction(const Event& event)
{
	const auto* p = std::get_if<ActionEvent>(&event.payload);
	if (!p) return;

	Framework& framework = Framework::Get();
	const Entity entity = framework.netIdRegistry.FindEntity(p->id);
	if (entity.IsNull()) return;

	auto* cmd = 
		_runtime.GetECS().GetStorage<EntityCommandFrame>().GetComponent(entity);
	if (!cmd) return;

	cmd->source = CommandSource::Player;

	switch (p->type) {
	case ActionRequestType::Attack:
	{
		if (!p->input) return;

		cmd->action.hasAction = true;
		cmd->action.actionType = ActionType::Attack;
		cmd->action.attackType = AttackType::Light;
		cmd->action.sequence++;
		break;
	}
	case ActionRequestType::Dodge:
	{
		if (!p->input) return;

		cmd->action.hasAction = true;
		cmd->action.actionType = ActionType::Dodge;
		cmd->action.attackType = AttackType::None;
		cmd->action.sequence++;
		break;
	}
	case ActionRequestType::Parry:
	{
		if (!p->input) return;

		cmd->action.hasAction = true;
		cmd->action.actionType = ActionType::Parry;
		cmd->action.attackType = AttackType::None;
		cmd->action.sequence++;
		break;
	}
	case ActionRequestType::Guard:
	{
		cmd->guard.hasGuard = true;
		cmd->guard.guardHeld = p->input;
		break;
	}
	default:
	{
		break;
	}
	}
}

XMFLOAT3 EventSystem::BuildMoveDirFromYawInput(
	const int inputX, 
	const int inputZ, 
	const float yaw)
{
	if (inputX == 0 && inputZ == 0)
		return XMFLOAT3{ 0, 0, 0 };

	const XMVECTOR forward = XMVectorSet(sin(yaw), 0, cos(yaw), 0);
	const XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward);

	XMVECTOR dir = XMVectorAdd(
		XMVectorScale(forward, static_cast<float>(inputZ)),
		XMVectorScale(right, static_cast<float>(inputX))
	);

	if (TransformHelper::SafeNormalize3(dir, dir))
	{
		XMFLOAT3 out;
		XMStoreFloat3(&out, dir);
		return out;
	}

	else
	{
		return XMFLOAT3{ 0, 0, 0 };
	}
}
