#pragma once

class IEventManager {
public:
	virtual ~IEventManager() = default;

public:
	virtual void Push(Event ev) = 0;
	virtual bool TryPop(Event& out) = 0;
};

class EventManager : public IEventManager {
public:
	EventManager() = default;
	virtual ~EventManager() = default;

	EventManager(const EventManager&) = delete;
	EventManager& operator=(const EventManager&) = delete;

	EventManager(EventManager&&) = delete;
	EventManager& operator=(EventManager&&) = delete;

public:
	virtual void Push(Event ev) override;
	virtual bool TryPop(Event& out) override;

private:
	concurrency::concurrent_priority_queue<Event> _eventQueue;
};