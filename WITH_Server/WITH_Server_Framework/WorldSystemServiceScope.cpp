#include "pch.h"
#include "WorldSystemServiceScope.h"

#include "ExecutionOps.h"
#include "NavMeshRuntime.h"
#include "TerrainHeightRuntime.h"
#include "WorldRuntime.h"

WorldSystemServiceScope::WorldSystemServiceScope(
    WorldRuntime& runtime,
    WorldSystemServices baseServices) noexcept
    : _services(baseServices)
{
    FillNavMeshProvider(runtime);
    FillTerrainHeightProvider(runtime);
}

WorldSystemServiceScope::WorldSystemServiceScope(
    NodeExecContext& context,
    WorldRuntime& runtime,
    WorldSystemServices baseServices) noexcept
    : _services(baseServices)
{
    FillNetBindingResolver(context);
    FillNavMeshProvider(runtime);
    FillTerrainHeightProvider(runtime);
}

void WorldSystemServiceScope::FillNavMeshProvider(WorldRuntime& runtime) noexcept
{
    if (_services.navMeshProvider != nullptr)
        return;

    const NavMeshRuntime* navMeshRuntime = runtime.GetNavMeshRuntime();
    if (navMeshRuntime == nullptr || !navMeshRuntime->IsReady())
        return;

    _navMeshProvider.Bind(navMeshRuntime, runtime.GetNavigationProfile());
    _services.navMeshProvider = &_navMeshProvider;
}

void WorldSystemServiceScope::FillTerrainHeightProvider(
    WorldRuntime& runtime) noexcept
{
    if (_services.terrainHeightProvider != nullptr)
        return;

    const TerrainHeightRuntime* terrainHeightRuntime =
        runtime.GetTerrainHeightRuntime();
    if (terrainHeightRuntime == nullptr || !terrainHeightRuntime->IsReady())
        return;

    _terrainHeightProvider.Bind(terrainHeightRuntime);
    _services.terrainHeightProvider = &_terrainHeightProvider;
}

void WorldSystemServiceScope::FillNetBindingResolver(
    NodeExecContext& context) noexcept
{
    if (_services.netBindingResolver != nullptr)
        return;

    _netBindingResolver.Bind(context);
    _services.netBindingResolver = &_netBindingResolver;
}

void WorldSystemServiceScope::NavMeshProvider::Bind(
    const NavMeshRuntime* runtime,
    const NavigationProfileDef* profile) noexcept
{
    _runtime = runtime;
    _profile = profile;
}

const NavMeshRuntime*
WorldSystemServiceScope::NavMeshProvider::GetNavMeshRuntime() const noexcept
{
    return _runtime;
}

const NavigationProfileDef*
WorldSystemServiceScope::NavMeshProvider::GetNavigationProfile() const noexcept
{
    return _profile;
}

void WorldSystemServiceScope::TerrainHeightProvider::Bind(
    const TerrainHeightRuntime* runtime) noexcept
{
    _runtime = runtime;
}

bool WorldSystemServiceScope::TerrainHeightProvider::TrySampleHeight(
    float worldX,
    float worldZ,
    float& outHeight) const noexcept
{
    if (_runtime == nullptr)
        return false;

    return _runtime->TrySampleHeight(worldX, worldZ, outHeight);
}

WorldSystemServiceScope::ExecContextNetBindingResolver::
ExecContextNetBindingResolver(const NodeExecContext* context) noexcept
    : _context(context)
{
}

void WorldSystemServiceScope::ExecContextNetBindingResolver::Bind(
    const NodeExecContext& context) noexcept
{
    _context = &context;
}

bool WorldSystemServiceScope::ExecContextNetBindingResolver::TryResolveEntity(
    const NetId& netId,
    Entity& outEntity) const noexcept
{
    outEntity = Entity::Null();

    if (_context == nullptr)
        return false;

    ExecutionOps* const ops = _context->TryGetOps();
    const WorldId worldId = _context->TryGetWorldId();
    if (ops == nullptr || !worldId.IsValid())
        return false;

    return ops->TryResolveEntity(worldId, netId, outEntity);
}
