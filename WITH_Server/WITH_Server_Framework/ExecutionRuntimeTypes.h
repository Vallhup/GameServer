#pragma once

#include <atomic>
#include <cstdint>
#include <span>
#include <vector>

#include "ExecutionCoreTypes.h"

struct ExecNodeRuntime
{
    std::atomic<ExecNodeState> state{ ExecNodeState::NotReady };
    std::atomic<uint32_t> remainingDeps{ 0 };

    void Reset(uint32_t depCount = 0) noexcept
    {
        state.store(ExecNodeState::NotReady);
        remainingDeps.store(depCount);
    }
};

struct ExecScopeRuntime
{
    std::atomic<ExecScopePhase> phase{ ExecScopePhase::Open };
    std::atomic<uint8_t> flags{ ExecScopeFlag_None };

    std::atomic<uint32_t> remainingNodes{ 0 };
    std::atomic<bool> closeCandidate{ false };

    void Reset(uint32_t nodeCount = 0) noexcept
    {
        phase.store(ExecScopePhase::Open);
        flags.store(ExecScopeFlag_None);
        remainingNodes.store(nodeCount);
        closeCandidate.store(false);
    }
};

struct ExecRuntimeSignals
{
    std::atomic<uint32_t> remainingSimulateNodes{ 0 };
    std::atomic<bool> simulatePhaseDone{ false };

    void Reset(uint32_t simulateNodeCount = 0) noexcept
    {
        remainingSimulateNodes.store(simulateNodeCount);
        simulatePhaseDone.store(false);
    }
};

struct ExecRuntimeState
{
    // [ Executor View ]
    std::span<ExecNodeRuntime> nodes;
    std::span<ExecScopeRuntime> scopes;
    ExecRuntimeSignals signals;

    void ClearViews() noexcept
    {
        nodes = {};
        scopes = {};
        signals.Reset(0);
    }

    void BindViews(
        std::span<ExecNodeRuntime> inNodes,
        std::span<ExecScopeRuntime> inScopes,
        uint32_t simulateNodeCount = 0) noexcept
    {
        nodes = inNodes;
        scopes = inScopes;
        signals.Reset(simulateNodeCount);
    }

    [[nodiscard]]
    bool IsValidNodeId(ExecNodeId nodeId) const noexcept
    {
        return 
            nodeId != InvalidExecNodeId &&
            nodeId < static_cast<ExecNodeId>(nodes.size());
    }

    [[nodiscard]]
    bool IsValidScopeId(ExecScopeId scopeId) const noexcept
    {
        return 
            scopeId != InvalidExecScopeId &&
            scopeId < static_cast<ExecScopeId>(scopes.size());
    }

    ExecNodeRuntime* TryGetNode(ExecNodeId nodeId) noexcept
    {
        return IsValidNodeId(nodeId) ? &nodes[nodeId] : nullptr;
    }

    const ExecNodeRuntime* TryGetNode(ExecNodeId nodeId) const noexcept
    {
        return IsValidNodeId(nodeId) ? &nodes[nodeId] : nullptr;
    }

    ExecScopeRuntime* TryGetScope(ExecScopeId scopeId) noexcept
    {
        return IsValidScopeId(scopeId) ? &scopes[scopeId] : nullptr;
    }

    const ExecScopeRuntime* TryGetScope(ExecScopeId scopeId) const noexcept
    {
        return IsValidScopeId(scopeId) ? &scopes[scopeId] : nullptr;
    }

    void ResetAll(
        std::span<const uint32_t> nodeDepCounts,
        std::span<const uint32_t> scopeNodeCounts,
        uint32_t simulateNodeCount) noexcept
    {
        const size_t nodeCount = nodes.size();
        const size_t scopeCount = scopes.size();

        for (size_t i = 0; i < nodeCount; ++i)
        {
            const uint32_t deps = (i < nodeDepCounts.size()) ? nodeDepCounts[i] : 0;
            nodes[i].Reset(deps);
        }

        for (size_t i = 0; i < scopeCount; ++i)
        {
            const uint32_t count = (i < scopeNodeCounts.size()) ? scopeNodeCounts[i] : 0;
            scopes[i].Reset(count);
        }

        signals.Reset(simulateNodeCount);
    }
};
