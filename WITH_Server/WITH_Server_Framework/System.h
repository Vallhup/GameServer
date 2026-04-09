#pragma once

#include "SystemMeta.h"
#include "ECSView.h"

class WorldRuntime;
class NetId;

struct IWorldNetBindingResolver
{
	virtual ~IWorldNetBindingResolver() = default;

	virtual bool TryResolveEntity(
		const NetId& netId,
		Entity& outEntity) const noexcept = 0;
};

struct WorldSystemServices
{
	const IWorldNetBindingResolver* netBindingResolver{ nullptr };
};

struct SystemContext
{
	WorldRuntime& runtime;
	ECSView ecs;
	double dtSec;
	WorldSystemServices services;
};

class System {
public:
	virtual ~System() = default;

	virtual void Execute(SystemContext& ctx) = 0;
	virtual const SystemMeta& Meta() const = 0;
};

template<typename T>
concept SysT = std::derived_from<T, System>;
