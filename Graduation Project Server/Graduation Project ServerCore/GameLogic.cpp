#include "pch.h"
#include "GameLogic.h"

GameLogic::GameLogic(Instance& instance, EventManager& eventMng)
	: _instance(instance), _eventMng(eventMng)
{
}

void GameLogic::LogicUpdate(float deltaTime)
{
	Event ev;
	while (_eventMng.TryPop(ev)) {
		if (ev.targetTime > std::chrono::high_resolution_clock::now()) {
			_eventMng.Push(std::move(ev));
			break;
		}

		ExecuteEvent(ev);
	}
}

void GameLogic::NetworkUpdate()
{
	// TODO : Logic Result Send
}

void GameLogic::OnPlayerAction(int sessionId, const std::vector<char>& packet)
{
	InputEventData data;
	Event ev{ EventType::Input, data, std::chrono::high_resolution_clock::now() };
	_eventMng.Push(std::move(ev));
}

void GameLogic::ExecuteEvent(Event event)
{
	try {
		switch (event.type) {
		case EventType::Input: {
			auto& data = std::get<InputEventData>(event.data);
			HandleInput(data);
			if (auto obj = _instance.GetGameObject(data.sessionId)) {
				if (auto inputComp = obj->GetComponent<InputComponent>()) {
					// TODO : Input에 맞는 Intent 처리
				}
			}

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
