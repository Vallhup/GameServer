#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ExecutionCoreTypes.h"

struct ExecRange
{
    uint32_t begin{ 0 };
    uint32_t count{ 0 };

    [[nodiscard]]
    constexpr uint32_t End() const noexcept
    {
        return begin + count;
    }

    [[nodiscard]]
    constexpr bool IsEmpty() const noexcept
    {
        return count == 0;
    }
};

struct ExecNodeRecord
{
    ExecNodeId id{ InvalidExecNodeId };
    ExecScopeId scopeId{ InvalidExecScopeId };

    ExecPhase phase{ ExecPhase::None };
    ExecLane lane{ ExecLane::None };
    ExecNodeKind kind{ ExecNodeKind::None };
    uint32_t flags{ ExecNodeFlag_None };

    uint32_t predBegin{ 0 };
    uint32_t predCount{ 0 };

    uint32_t succBegin{ 0 };
    uint32_t succCount{ 0 };

    uint32_t debugNameOffset{ 0 };
    ExecToken sourceToken{ InvalidExecToken };
};

struct BuildDiagnostic
{
    BuildDiagnosticSeverity severity{ BuildDiagnosticSeverity::Info };
    std::string message;
};

// WorldId로 치환 가능
using WorldBinding = uint64_t;

struct FrameTaskGraph
{
    // Immutable node records built for this frame.
    std::vector<ExecNodeRecord> nodes;

    // Shared edge pool referenced by pred/succ begin+count.
    std::vector<ExecNodeId> edges;

    // scopeId -> world/runtime binding key.
    // 1차 구현에서는 WorldId 또는 그에 준하는 handle을 담는 쪽이 자연스럽다.
    std::vector<WorldBinding> scopeToWorld;

    // Stable topological order for serial phases.
    std::vector<ExecNodeId> serialExecutionOrder;

    ExecRange commitPlan;
    ExecRange lifecycleFlushPlan;
    ExecRange reconcilePlan;

    uint32_t simulateNodeCount{ 0 };
    uint32_t scopeCount{ 0 };

    std::string debugNameBlob;

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
            scopeId < static_cast<ExecScopeId>(scopeCount) &&
            scopeId < static_cast<ExecScopeId>(scopeToWorld.size());
    }

    [[nodiscard]]
    bool IsValidSerialRange(const ExecRange& range) const noexcept
    {
        return 
            range.begin <= serialExecutionOrder.size() &&
            range.End() <= serialExecutionOrder.size();
    }

    [[nodiscard]]
    bool IsValidPredRange(const ExecNodeRecord& node) const noexcept
    {
        return 
            node.predBegin <= edges.size() &&
            node.predBegin + node.predCount <= edges.size();
    }

    [[nodiscard]]
    bool IsValidSuccRange(const ExecNodeRecord& node) const noexcept
    {
        return 
            node.succBegin <= edges.size() &&
            node.succBegin + node.succCount <= edges.size();
    }

    [[nodiscard]]
    bool HasSerialPlan() const noexcept
    {
        return !serialExecutionOrder.empty();
    }

    void Clear()
    {
        nodes.clear();
        edges.clear();
        scopeToWorld.clear();
        serialExecutionOrder.clear();

        commitPlan = {};
        lifecycleFlushPlan = {};
        reconcilePlan = {};

        simulateNodeCount = 0;
        scopeCount = 0;
    }
};

struct BuildResult
{
    FrameTaskGraph graph;
    std::vector<BuildDiagnostic> diagnostics;

    // succcess == !HasError()
    // builder가 마지막에 명시적으로 세팅
    bool success{ false };

    [[nodiscard]]
    bool HasError() const noexcept
    {
        for (const BuildDiagnostic& diag : diagnostics)
        {
            if (diag.severity == BuildDiagnosticSeverity::Error)
                return true;
        }
        return false;
    }
};