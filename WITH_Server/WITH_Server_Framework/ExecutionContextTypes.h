#pragma once

#include <cstdint>
#include <span>

#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionOps.h"

struct WorldFrameSelectionSet;
struct ExecutionGraphBuildPolicy;

class WorldRuntime;
class ExecutionSourceRegistry;
class WorldExecutionModelRegistry;

struct NodeScratch
{
    void* memory{ nullptr };
    uint32_t size{ 0 };
};

struct FrameBuildContext
{
    const WorldFrameSelectionSet* frameSelectionSet{ nullptr };
    const WorldExecutionModelRegistry* executionModelRegistry{ nullptr };
    const ExecutionSourceRegistry* executionSourceRegistry{ nullptr };
    const ExecutionGraphBuildPolicy* buildPolicy{ nullptr };

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return 
            frameSelectionSet != nullptr &&
            executionModelRegistry != nullptr &&
            executionSourceRegistry != nullptr &&
            buildPolicy != nullptr;
    }
};

struct FrameExecContext
{
    const FrameTaskGraph* graph{ nullptr };
    ExecutionOps* ops{ nullptr };
    std::span<WorldRuntime*> runtimeByScope{};

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            graph != nullptr &&
            ops != nullptr &&
            ops->IsValid();
    }

    [[nodiscard]]
    bool IsValidScopeId(ExecScopeId scopeId) const noexcept
    {
        return 
            scopeId != InvalidExecScopeId &&
            scopeId < static_cast<ExecScopeId>(runtimeByScope.size());
    }

    [[nodiscard]]
    WorldRuntime* TryGetRuntime(ExecScopeId scopeId) const noexcept
    {
        return (ops != nullptr)
            ? ops->GetRuntime(scopeId, runtimeByScope)
            : nullptr;
    }
};

struct NodeExecContext
{
    FrameExecContext* frame{ nullptr };
    ExecNodeId nodeId{ InvalidExecNodeId };
    ExecScopeId scopeId{ InvalidExecScopeId };
    ExecToken sourceToken{ InvalidExecToken };
    NodeScratch* scratch{ nullptr };

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return 
            frame != nullptr &&
            frame->IsValid() &&
            nodeId != InvalidExecNodeId &&
            scopeId != InvalidExecScopeId;
    }

    [[nodiscard]]
    WorldRuntime* TryGetRuntime() const noexcept
    {
        return (frame != nullptr) ? frame->TryGetRuntime(scopeId) : nullptr;
    }

    [[nodiscard]]
    ExecutionOps* TryGetOps() const noexcept
    {
        return (frame != nullptr) ? frame->ops : nullptr;
    }
};

using ExecFn = ExecCallResult(*)(NodeExecContext&);
