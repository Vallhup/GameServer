#pragma once

#include <variant>

#include "InputSystem.h"
#include "BTNode.h"

enum class EventType {
	BT,
	Input,
	Timer
};

struct InputEventData {
	int sessionId;
	TestInput key;
	TestInputType type;
};

struct TimerEventData {
	int objectId;
	std::function<void()> func;
};

struct BTEventData {
	std::function<NodeStatus()> func;
	std::shared_ptr<std::promise<NodeStatus>> promise;
};

using EventData = std::variant<InputEventData, TimerEventData, BTEventData>;

struct Event {
	EventType type;
	EventData data;
	std::chrono::high_resolution_clock::time_point targetTime;

	bool operator<(const Event& other) const
	{
		return targetTime > other.targetTime;
	}
};