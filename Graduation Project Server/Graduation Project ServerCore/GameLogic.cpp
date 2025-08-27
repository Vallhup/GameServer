#include "pch.h"
#include "GameLogic.h"

GameLogic::GameLogic(Instance* instance, IEventManager* eventMng)
	: _instance(instance), _eventMng(eventMng)
{
}

void GameLogic::LogicUpdate(float deltaTime)
{
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
	LOG_DBG("GameLogic NetworkUpdate");

	auto objList = _instance->GetGameObjectList();

	for (auto& obj : objList) {
		if (auto trComp = obj->GetComponent<TransformComponent>()) {
			if (trComp->VersionCheckAndChange()) {
				LOG_DBG("GameLogic NetworkUpdate");

				vec3 pos = trComp->GetPosition();

				Protocol::Vec3 protoPos;
				protoPos.set_x(pos.x);
				protoPos.set_y(pos.y);
				protoPos.set_z(pos.z);

				_instance->BroadCast(PacketFactory::SCMovePakcet(obj->GetId(), protoPos));
			}

			else {
				LOG_DBG("Version not change");
			}
		}
	}
}

void GameLogic::OnPlayerAction(int sessionId, const Protocol::CS_INPUT_PACKET& packet)
{
	InputEventData data{ sessionId, packet.key(), packet.inputtype() };
	Event ev{ EventType::Input, data, std::chrono::high_resolution_clock::now() };
	_eventMng->Push(std::move(ev));
}

void GameLogic::ExecuteEvent(Event event)
{
	try {
		switch (event.type) {
		case EventType::Input: {
			auto& data = std::get<InputEventData>(event.data);
			HandleInput(data);
			break;
		}
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

void GameLogic::HandleInput(const InputEventData& data)
{
	static const std::array<vec3, 4> dirs = {
		vec3{0.0f,  0.0f, 1.0f},
		vec3{-1.0f, 0.0f, 0.0f},
		vec3{0.0f, 0.0f, -1.0f},
		vec3{1.0f, 0.0f,  0.0f}
	};

	if (auto obj = _instance->GetGameObject(data.sessionId)) {
		if (auto inputComp = obj->GetComponent<InputComponent>()) {
			
			vec3 dir{ 0, 0, 0 };
			for (int i = Protocol::MOVE_FRONT; i <= Protocol::MOVE_RIGHT; ++i) {
				if (inputComp->IsKeyDown((Protocol::Input)(i - 1))) {
					dir += dirs[i - Protocol::MOVE_FRONT];
				}
			}

			if (auto moveComp = obj->GetComponent<MovementComponent>()) {
				moveComp->SetVelocity(dir);
				moveComp->SetEnable(true);
			}
		}
	}
}
