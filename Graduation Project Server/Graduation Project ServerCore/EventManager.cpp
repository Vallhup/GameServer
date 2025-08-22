#include "pch.h"
#include "EventManager.h"

EventManager::EventManager() : _running(false)
{
}

EventManager::~EventManager()
{
	Stop();
}

bool EventManager::Start()
{
	bool expected{ false };
	if (_running.compare_exchange_strong(expected, true)) {
		_oneTimeTaskThread = std::thread([this]() { this->EventThreadLoop(); });
		return true;
	}

	return false;
}

void EventManager::Stop()
{
	bool expected{ true };
	if (_running.compare_exchange_strong(expected, false)) {
		if (_oneTimeTaskThread.joinable()) {
			_oneTimeTaskThread.join();
		}
	}
}

std::future<EventReturn> EventManager::AddEvent(const EventFunc& func, float delayMs)
{
	using namespace std::chrono;

	Event event{ func, high_resolution_clock::now() + milliseconds(static_cast<int>(delayMs)) };
	
	auto future = event.promise->get_future();
	_eventQueue.push(std::move(event));

	return future;
}

void EventManager::EventThreadLoop()
{
	using namespace std::chrono;

	while (_running) {
		Event event;
		while (_eventQueue.try_pop(event)) {
			if (event.targetTime > high_resolution_clock::now()) {
				_eventQueue.push(event);
				break;
			}

			try {
				auto future = event.func();
				event.promise->set_value(future);
			}
			catch (...) {
				event.promise->set_exception(std::current_exception());
			}
		}
	}
}