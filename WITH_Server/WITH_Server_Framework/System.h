#pragma once

#include <vector>
#include <typeindex>

class ECS;

class System {
public:
	System(ECS& e, int p = 0) : ecs(e), _priority(p) {}
	virtual ~System() = default;

	virtual void Execute(const float dT) = 0;

	virtual std::vector<std::type_index> ReadResources() const = 0;
	virtual std::vector<std::type_index> WriteResources() const = 0;

	int GetPriority() const { return _priority; }

	// SystemManager에 등록될 때 초기화
	int stableOrder{ -1 };

protected:
	ECS& ecs;
	int _priority;
};