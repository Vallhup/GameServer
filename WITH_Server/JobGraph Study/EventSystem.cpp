#include "pch.h"
#include "EventSystem.h"

#include "Framework.h"
#include "Entity.h"
#include "Component.h"

EventSystem::EventSystem(ECS& e, int p) : System(e, p) 
{
	_handlers[EventType::EV_CONNECT] = [&](const Event& ev) { ProcessConnect(ev); };
	_handlers[EventType::EV_DISCONNECT] = [&](const Event& ev) { ProcessDisconnect(ev); };
	_handlers[EventType::EV_MOVE] = [&](const Event& ev) { ProcessMove(ev); };
	_handlers[EventType::EV_ACTION] = [&](const Event& ev) { ProcessAction(ev); };
}

void EventSystem::Execute(const float dT)
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

	Entity entity = ecs.entityMng.CreatePlayer(p->sessionId);

	// TODO : Entity¿¡ Component Ãß°¡
	ecs.GetStorage<Transform>().AddComponent(entity);
	ecs.GetStorage<Velocity>().AddComponent(entity);
	ecs.GetStorage<LocomotionState>().AddComponent(entity);
	ecs.GetStorage<ActionIntent>().AddComponent(entity);
	ecs.GetStorage<ActionState>().AddComponent(entity);
	ecs.GetStorage<Health>().AddComponent(entity);
	ecs.GetStorage<AnimationState>().AddComponent(entity);
	auto animRef = ecs.GetStorage<AnimationRef>().AddComponent(entity);
	ecs.GetStorage<Animator>().AddComponent(entity);
	ecs.GetStorage<Collider>().AddComponent(entity);

	animRef->anim = AnimationManager::Get().GetAnimation(AnimationId::Knight_Idle);

	auto& ets = Framework::Get().entityToSession;
	auto it = ets.find(entity);
	if (it != ets.end())
		Framework::Get().outEventQueue.push(OutputEvent{ entity, DirtyType::Spawned });
}

void EventSystem::ProcessDisconnect(const Event& event)
{
	const auto* p = std::get_if<DisconnectEvent>(&event.payload);

#ifdef _DEBUG
	std::cout << "[EventSystem] Player[" << p->sessionId << "] Disconnect\n";
#endif

	auto& ste = Framework::Get().sessionToEntity;

	auto it = ste.find(p->sessionId);
	if (it == ste.end()) return;
	Entity entity = it->second;

	ecs.GetStorage<DisconnectedTag>().AddComponent(entity);
	Framework::Get().outEventQueue.push(OutputEvent{ entity, DirtyType::Despawned });
}

void EventSystem::ProcessMove(const Event& event)
{
	const auto* p = std::get_if<MoveEvent>(&event.payload);
	if (!p) return;

	auto it = Framework::Get().sessionToEntity.find(p->sessionId);
	if (it == Framework::Get().sessionToEntity.end()) return;
	Entity entity = it->second;

	if (auto* velocity = ecs.GetStorage<Velocity>().GetComponent(entity))
	{
		if (auto* loco = ecs.GetStorage<LocomotionState>().GetComponent(entity))
		{
			int inputX = p->inputX;
			int inputZ = p->inputZ;
			float yaw = p->yaw;

			XMVECTOR forward = XMVectorSet(sin(yaw), 0, cos(yaw), 0);
			XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward);

			XMVECTOR dir = XMVectorAdd(
				XMVectorScale(forward, inputZ),
				XMVectorScale(right, inputX)
			);

			if (p->inputX == 0 && p->inputZ == 0)
			{
				loco->isMoving = false;
				dir = XMVectorZero();
			}

			else
			{
				loco->isMoving = true;
				dir = XMVector3Normalize(dir);
			}

			XMStoreFloat3(&velocity->dir, dir);
		}
	}
}

void EventSystem::ProcessAction(const Event& event)
{
	const auto* p = std::get_if<ActionEvent>(&event.payload);
	if (!p) return;

	auto it = Framework::Get().sessionToEntity.find(p->sessionId);
	if (it == Framework::Get().sessionToEntity.end()) return;
	Entity entity = it->second;

	if (auto* actionIntent = ecs.GetStorage<ActionIntent>().GetComponent(entity))
	{
		actionIntent->attack = p->attack;
		actionIntent->dodge = p->dodge;
		actionIntent->parry = p->parry;
		actionIntent->guard = p->guard;

		if (auto* actionState = ecs.GetStorage<ActionState>().GetComponent(entity))
		{
			if (actionState->type == ActionType::Guard && !p->guard)
			{
				ecs.GetStorage<ActionRequestTag>().AddComponent(entity)->type = ActionType::None;
			}
		}
	}
}
