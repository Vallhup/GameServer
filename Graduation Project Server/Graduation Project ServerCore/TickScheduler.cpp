#include "pch.h"
#include "TickScheduler.h"

void TickScheduler::Register(ITickable* t)
{
	if (std::find(_tickables.begin(), _tickables.end(), t) == _tickables.end()) {
		_tickables.emplace_back(t);
	}
}

void TickScheduler::Deregister(ITickable* t)
{
	std::erase(_tickables, t);
}

void TickScheduler::Tick(float deltaTime)
{
	for (auto tickable : _tickables) {
		if(tickable->TickEnable()) {
			tickable->Tick(deltaTime);
		}
	}
}
