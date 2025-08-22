#pragma once

class ITickable;

class TickSystem {
public:
	void Register(ITickable* t);
	void Deregister(ITickable* t);

	void Tick(float deltaTime);

private:
	std::vector<ITickable*> _tickables;
};