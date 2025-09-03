#pragma once

#include <variant>

#include "BTNode.h"
#include "Protocols/Enum.pb.h"
#include "Protocols/Struct.pb.h"

enum class EventType {
	BT,
	Timer
};

struct TimerEventData {
	int objectId;
	std::function<void()> func;
};

struct BTEventData {
	std::function<NodeStatus()> func;
	std::shared_ptr<std::promise<NodeStatus>> promise;
};

using EventData = std::variant<TimerEventData, BTEventData>;

struct Event {
	EventType type;
	EventData data;
	std::chrono::high_resolution_clock::time_point targetTime;

	bool operator<(const Event& other) const
	{
		return targetTime > other.targetTime;
	}
};