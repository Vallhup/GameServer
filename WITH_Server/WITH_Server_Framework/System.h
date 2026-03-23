#pragma once

#include "SystemMeta.h"

class WorldRuntime;

class System {
public:
	System(WorldRuntime& rt) : _runtime(rt) {}
	virtual ~System() = default;

	virtual void Execute(const double dT) = 0;
	virtual const SystemMeta& Meta() const = 0;

protected:
	WorldRuntime& _runtime;
};

template<typename T>
concept SysT = std::derived_from<T, System>;