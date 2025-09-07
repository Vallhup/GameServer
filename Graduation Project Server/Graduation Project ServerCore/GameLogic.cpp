#include "pch.h"
#include "GameLogic.h"

GameLogic::GameLogic(Instance* instance, IEventManager* eventMng)
	: _instance(instance), _eventMng(eventMng)
{
}

void GameLogic::LogicUpdate(float deltaTime)
{
	for (auto& obj : _instance->GetGameObjectList()) {
		obj->Update(deltaTime);
	}

	Event ev;
	while (_eventMng->TryPop(ev)) {
		if (ev.targetTime > std::chrono::high_resolution_clock::now()) {
			_eventMng->Push(std::move(ev));
			break;
		}

		ExecuteEvent(ev);
	}
}

void GameLogic::NetworkUpdate()
{
	auto objList = _instance->GetGameObjectList();

	for (auto& obj : objList) {
		if (auto trComp = obj->GetComponent<TransformComponent>()) {
			if (trComp->VersionCheckAndChange()) {
				vec3 pos = trComp->GetPosition();

				Protocol::Vec3 protoPos;
				protoPos.set_x(pos.x);
				protoPos.set_y(pos.y);
				protoPos.set_z(pos.z);

				_instance->BroadCast(PacketFactory::SCMovePakcet(obj->GetId(), protoPos));
				LOG_DBG("Send Move Packet");
			}
		}
	}
}

void GameLogic::OnPlayerAction(int sessionId, Protocol::CS_INPUT_PACKET& packet)
{
	_instance->GetGameObject(sessionId)->GetComponent<InputComponent>()->Enqueue(packet);
}

void GameLogic::ExecuteEvent(Event event)
{
	try {
		switch (event.type) {
		case EventType::Timer: {
			auto& data = std::get<TimerEventData>(event.data);
			data.func();
			break;
		}
		case EventType::BT: {
			// TEMP : 추후 Worker Thread로 작업 넘길 예정
			auto& data = std::get<BTEventData>(event.data);
			auto result = data.func();
			data.promise->set_value(result);
			break;
		}
		}
	}
		
	catch (...) {
		if (event.type == EventType::BT) {
			auto& data = std::get<BTEventData>(event.data);
			data.promise->set_exception(std::current_exception());
		}
	}
}