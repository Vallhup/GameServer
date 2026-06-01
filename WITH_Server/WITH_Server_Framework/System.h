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

struct ITerrainHeightProvider
{
	virtual ~ITerrainHeightProvider() = default;

	virtual bool TrySampleHeight(
		float worldX,
		float worldZ,
		float& outHeight) const noexcept = 0;
};

struct WorldSystemServices
{
	const IWorldNetBindingResolver* netBindingResolver{ nullptr };
	const INavMeshProvider*         navMeshProvider{ nullptr };
	const ITerrainHeightProvider*   terrainHeightProvider{ nullptr };
};

struct SystemContext
{
	WorldRuntime& runtime;
	ECSView ecs;
	double dtSec;
	WorldSystemServices services;
	// 현재 프레임에서 이 시스템이 실행 중인 월드의 스코프 ID.
	// DB 커맨드 제출 시 DBRequestMeta.scopeId에 사용한다.
	ExecScopeId execScopeId{ InvalidExecScopeId };
};

class System {
public:
	virtual ~System() = default;

	virtual void Execute(SystemContext& ctx) = 0;
	virtual const ExecMeta& Meta() const = 0;
};

template<typename T>
concept SysT = std::derived_from<T, System>;
