#pragma once

#include "SystemMeta.h"
#include "ECSView.h"

class WorldRuntime;

struct SystemContext
{
	WorldRuntime& runtime;
	ECSView ecs;
	double dtSec;
};

class System {
public:
	virtual ~System() = default;

	virtual void Execute(SystemContext& ctx) = 0;
	virtual const SystemMeta& Meta() const = 0;
};

template<typename T>
concept SysT = std::derived_from<T, System>;