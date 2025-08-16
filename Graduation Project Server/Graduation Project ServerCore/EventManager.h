#pragma once

#include <variant>
#include "BTNode.h"

using EventReturn = std::variant<std::monostate, NodeStatus>;
using EventFunc = std::function<EventReturn()>;

struct Event {
	EventFunc func;
	std::chrono::high_resolution_clock::time_point targetTime;
	std::shared_ptr<std::promise<EventReturn>> promise;
	// MSVC의 거지같은 구현 오류를 피하기 위한 온몸비틀기

	bool operator<(const Event& other) const
	{
		return targetTime > other.targetTime;
	}
};

class IEventManager {
public:
	virtual ~IEventManager() = default;

public:
	virtual bool Start() = 0;
	virtual void Stop() = 0;

	virtual std::future<EventReturn> AddEvent(const EventFunc& func, float delayMs) = 0;
};

class EventManager : public IEventManager {
public:
	EventManager();
	virtual ~EventManager();

	EventManager(const EventManager&) = delete;
	EventManager& operator=(const EventManager&) = delete;

	EventManager(EventManager&&) = delete;
	EventManager& operator=(EventManager&&) = delete;

public:
	virtual bool Start() override;
	virtual void Stop() override;

	virtual std::future<EventReturn> AddEvent(const EventFunc& func, float delayMs) override;

private:
	void EventThreadLoop();

private:
	std::atomic<bool> _running;

	concurrency::concurrent_priority_queue<Event> _eventQueue;

	std::thread _oneTimeTaskThread;
};

