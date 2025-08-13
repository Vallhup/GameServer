#pragma once

class IEventManager {
public:
	virtual ~IEventManager() = default;

public:
	virtual bool Start() = 0;
	virtual void Stop() = 0;

	virtual void AddEvent(const std::function<void()>& func, float delayMs) = 0;
};

class EventManager : public IEventManager {
	struct Event {
		std::function<void()> func;
		std::chrono::high_resolution_clock::time_point targetTime;

		bool operator<(const Event& other) const
		{
			return targetTime > other.targetTime;
		}
	};

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

	virtual void AddEvent(const std::function<void()>& func, float delayMs) override;

private:
	void EventThreadLoop();

private:
	std::atomic<bool> _running;

	concurrency::concurrent_priority_queue<Event> _eventQueue;

	std::thread _oneTimeTaskThread;
};

