#pragma once

#include <atomic>
#include <chrono>
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

// 프레임 단위 비동기 IO 상태.
// TaskExecutor가 소유. 프레임 시작 시 Reset() 호출.
struct FrameState
{
    using TimePoint = std::chrono::steady_clock::time_point;

    // 스윕 식별자. TriggerSweep 시 1 증가. ProcessCompletions에서 epoch 불일치 시 무시.
    std::atomic<uint32_t> epoch{ 0 };

    TimePoint simulateDeadline{};

    // Suspended 노드 ID 배열. 프레임당 최대 kMaxSuspendedPerFrame개.
    // pre-allocated — 동적 할당 없음.
    static constexpr uint32_t kMaxSuspendedPerFrame = 4096;
    ExecNodeId suspendedBuffer[kMaxSuspendedPerFrame]{};
    std::atomic<uint32_t> suspendedTail{ 0 };

    // TriggerSweep의 단일 실행 보장 (CAS 기반).
    std::atomic<bool> sweepTriggered{ false };

    void Reset(TimePoint deadline) noexcept
    {
        epoch.fetch_add(1, std::memory_order_relaxed);
        simulateDeadline = deadline;
        suspendedTail.store(0, std::memory_order_relaxed);
        sweepTriggered.store(false, std::memory_order_relaxed);
    }

    // IO 대기 노드 등록. 배열 초과 시 false 반환 (호출부에서 직접 Canceled 처리).
    bool TryPushSuspended(ExecNodeId nodeId) noexcept
    {
        const uint32_t idx = suspendedTail.fetch_add(1, std::memory_order_relaxed);
        if (idx >= kMaxSuspendedPerFrame)
        {
            suspendedTail.fetch_sub(1, std::memory_order_relaxed);
            return false;
        }
        suspendedBuffer[idx] = nodeId;
        return true;
    }

    [[nodiscard]]
    bool IsDeadlineExceeded() const noexcept
    {
        return std::chrono::steady_clock::now() >= simulateDeadline;
    }
};
