#include "pch.h"
#include "TickSystem.h"

void TickSystem::Register(ITickable* t)
{
	if (std::find(_tickables.begin(), _tickables.end(), t) == _tickables.end()) {
		_tickables.emplace_back(t);
	}
}

void TickSystem::Deregister(ITickable* t)
{
	std::erase(_tickables, t);
}

void TickSystem::Tick(float deltaTime)
{
	for (auto tickable : _tickables) {
		if(tickable->TickEnable()) {
			tickable->Tick(deltaTime);
		}
	}
}
