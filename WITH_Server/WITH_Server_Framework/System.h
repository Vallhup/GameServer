#pragma once

#include <vector>
#include <typeindex>

class WorldRuntime;

class System {
public:
	System(WorldRuntime& rt, int p = 0) : _runtime(rt), _priority(p) {}
	virtual ~System() = default;

	virtual void Execute(const double dT) = 0;

	virtual std::vector<std::type_index> ReadResources() const = 0;
	virtual std::vector<std::type_index> WriteResources() const = 0;

	int GetPriority() const { return _priority; }

	// SystemManager에 등록될 때 초기화
	int stableOrder{ -1 };

protected:
	WorldRuntime& _runtime;
	int _priority;
};