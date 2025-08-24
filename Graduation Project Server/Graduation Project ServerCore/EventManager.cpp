#include "pch.h"
#include "EventManager.h"

void EventManager::Push(Event ev)
{
	_eventQueue.push(std::move(ev));
}

bool EventManager::TryPop(Event& out)
{
	return _eventQueue.try_pop(out);
}