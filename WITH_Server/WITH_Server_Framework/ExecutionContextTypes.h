#pragma once

#include <cstdint>
#include <span>

#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"   // DynamicTaskFrameTable 포함
#include "ExecutionOps.h"
#include "WorldId.h"

struct WorldFrameSelectionSet;
struct ExecutionGraphBuildPolicy;
struct DynamicTaskFrozenBatch;

class WorldRuntime;
class ExecutionSourceRegistry;
class WorldExecutionModelRegistry;
class ConflictRegistry;
class DynamicTaskTypeRegistry;
class DynamicTaskFrameTable;

struct NodeScratch
{
    void* memory{ nullptr };
    uint32_t size{ 0 };
};

struct FrameBuildContext
{
    const WorldFrameSelectionSet*      frameSelectionSet{ nullptr };
    const WorldExecutionModelRegistry* executionModelRegistry{ nullptr };
    const ExecutionSourceRegistry*     executionSourceRegistry{ nullptr };
    const ExecutionGraphBuildPolicy*   buildPolicy{ nullptr };

    // 사용자 정의 ResourceKind 충돌 정책 (optional).
    // nullptr 이면 빌트인 5-rule 알고리즘(ConflictDetection.h)만 사용한다.
    const ConflictRegistry*            conflictRegistry{ nullptr };

    // Dynamic Task 확장 (optional).
    // 두 포인터 모두 nullptr이면 이 프레임에서 동적 노드를 추가하지 않는다.
    // 하나만 nullptr이면 빌더가 경고를 발생시킨다.
    const DynamicTaskFrozenBatch*      dynamicBatch{ nullptr };
    const DynamicTaskTypeRegistry*     dynamicTaskTypeRegistry{ nullptr };

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            frameSelectionSet != nullptr &&
            executionModelRegistry != nullptr &&
            executionSourceRegistry != nullptr &&
            buildPolicy != nullptr;
        // conflictRegistry, dynamicBatch, dynamicTaskTypeRegistry는
        // optional이므로 유효성 검사에서 제외한다.
    }
};

struct FrameExecContext
{
    const FrameTaskGraph* graph{ nullptr };
    ExecutionOps* ops{ nullptr };
    std::span<WorldRuntime*> runtimeByScope{};
    std::span<const WorldId> worldIdByScope{};

    // Dynamic Task 확장 (optional).
    // BuildResult::dynamicTaskFrameTable을 가리킨다.
    // 동적 노드가 없는 프레임에서는 nullptr이다.
    DynamicTaskFrameTable* dynamicTaskFrameTable{ nullptr };

    void Clear() noexcept
    {
        graph = nullptr;
        ops = nullptr;
        runtimeByScope = {};
        worldIdByScope = {};
        dynamicTaskFrameTable = nullptr;
    }

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
            scopeId < static_cast<ExecScopeId>(runtimeByScope.size()) &&
            scopeId < static_cast<ExecScopeId>(worldIdByScope.size());
    }

    [[nodiscard]]
    WorldRuntime* TryGetRuntime(ExecScopeId scopeId) const noexcept
    {
        return (ops != nullptr)
            ? ops->GetRuntime(scopeId, runtimeByScope)
            : nullptr;
    }

    [[nodiscard]]
    WorldId TryGetWorldId(ExecScopeId scopeId) const noexcept
    {
        return IsValidScopeId(scopeId)
            ? worldIdByScope[scopeId]
            : WorldId::Invalid();
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
    WorldId TryGetWorldId() const noexcept
    {
        return (frame != nullptr)
            ? frame->TryGetWorldId(scopeId)
            : WorldId::Invalid();
    }

    [[nodiscard]]
    ExecutionOps* TryGetOps() const noexcept
    {
        return (frame != nullptr) ? frame->ops : nullptr;
    }
};

using ExecFn = ExecCallResult(*)(NodeExecContext&);
