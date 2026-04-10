#pragma once

#include "SystemMeta.h"
#include "ECSView.h"

class WorldRuntime;
class NetId;
class NavMeshRuntime;
struct NavigationProfileDef;

struct IWorldNetBindingResolver
{
	virtual ~IWorldNetBindingResolver() = default;

	virtual bool TryResolveEntity(
		const NetId& netId,
		Entity& outEntity) const noexcept = 0;
};

// NavMesh 런타임 및 프로파일에 대한 접근 인터페이스.
// WorldRuntime이 자동으로 WorldSystemServices에 주입하므로
// 시스템은 ctx.services.navMeshProvider를 통해 접근한다.
struct INavMeshProvider
{
	virtual ~INavMeshProvider() = default;

	virtual const NavMeshRuntime*       GetNavMeshRuntime()    const noexcept = 0;
	virtual const NavigationProfileDef* GetNavigationProfile() const noexcept = 0;
};

struct WorldSystemServices
{
	const IWorldNetBindingResolver* netBindingResolver{ nullptr };
	const INavMeshProvider*         navMeshProvider{ nullptr };
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
