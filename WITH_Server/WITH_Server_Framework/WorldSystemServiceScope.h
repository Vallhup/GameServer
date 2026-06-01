#pragma once

#include "ExecutionContextTypes.h"
#include "System.h"

class WorldRuntime;
class TerrainHeightRuntime;

class WorldSystemServiceScope final
{
public:
    explicit WorldSystemServiceScope(
        WorldRuntime& runtime,
        WorldSystemServices baseServices = {}) noexcept;

    WorldSystemServiceScope(
        NodeExecContext& context,
        WorldRuntime& runtime,
        WorldSystemServices baseServices = {}) noexcept;

    WorldSystemServiceScope(const WorldSystemServiceScope&) = delete;
    WorldSystemServiceScope& operator=(const WorldSystemServiceScope&) = delete;

    [[nodiscard]]
    const WorldSystemServices& Services() const noexcept
    {
        return _services;
    }

private:
    class NavMeshProvider final : public INavMeshProvider
    {
    public:
        void Bind(
            const NavMeshRuntime* runtime,
            const NavigationProfileDef* profile) noexcept;

        [[nodiscard]]
        const NavMeshRuntime* GetNavMeshRuntime() const noexcept override;

        [[nodiscard]]
        const NavigationProfileDef* GetNavigationProfile() const noexcept override;

    private:
        const NavMeshRuntime* _runtime{ nullptr };
        const NavigationProfileDef* _profile{ nullptr };
    };

    class TerrainHeightProvider final : public ITerrainHeightProvider
    {
    public:
        void Bind(const TerrainHeightRuntime* runtime) noexcept;

        [[nodiscard]]
        bool TrySampleHeight(
            float worldX,
            float worldZ,
            float& outHeight) const noexcept override;

    private:
        const TerrainHeightRuntime* _runtime{ nullptr };
    };

    class ExecContextNetBindingResolver final : public IWorldNetBindingResolver
    {
    public:
        explicit ExecContextNetBindingResolver(
            const NodeExecContext* context = nullptr) noexcept;

        void Bind(const NodeExecContext& context) noexcept;

        [[nodiscard]]
        bool TryResolveEntity(
            const NetId& netId,
            Entity& outEntity) const noexcept override;

    private:
        const NodeExecContext* _context{ nullptr };
    };

private:
    void FillNavMeshProvider(WorldRuntime& runtime) noexcept;
    void FillTerrainHeightProvider(WorldRuntime& runtime) noexcept;
    void FillNetBindingResolver(NodeExecContext& context) noexcept;

private:
    WorldSystemServices _services{};
    NavMeshProvider _navMeshProvider;
    TerrainHeightProvider _terrainHeightProvider;
    ExecContextNetBindingResolver _netBindingResolver;
};
