#include "pch.h"
#include "WorldSystemServiceScope.h"

#include "ExecutionOps.h"
#include "NavMeshRuntime.h"
#include "WorldRuntime.h"

WorldSystemServiceScope::WorldSystemServiceScope(
    WorldRuntime& runtime,
    WorldSystemServices baseServices) noexcept
    : _services(baseServices)
{
    FillNavMeshProvider(runtime);
}

WorldSystemServiceScope::WorldSystemServiceScope(
    NodeExecContext& context,
    WorldRuntime& runtime,
    WorldSystemServices baseServices) noexcept
    : _services(baseServices)
{
    FillNetBindingResolver(context);
    FillNavMeshProvider(runtime);
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
