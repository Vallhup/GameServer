#pragma once

#include <cstdint>
#include <span>

#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"   // DynamicTaskFrameTable 포함
#include "ExecutionOps.h"
#include "WorldId.h"
#include "AsyncIOTypes.h"          // IoHandle

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

// IoHandle 풀 획득 인터페이스. TaskExecutor가 구현.
// InvokeNode() 직전에 NodeExecContext::ioProvider로 주입된다.
struct IIoHandleProvider
{
    // 풀에서 IoHandle 획득. nodeId·epoch 설정, refCount = 2 초기화.
    // nullptr 반환 시 풀 고갈 → 호출부에서 ExecCallResult::Failed 처리.
    virtual IoHandle* Acquire(ExecNodeId nodeId, uint32_t epoch) noexcept = 0;
    virtual ~IIoHandleProvider() = default;
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
    FrameExecContext*  frame{ nullptr };
    ExecNodeId         nodeId{ InvalidExecNodeId };
    ExecScopeId        scopeId{ InvalidExecScopeId };
    ExecToken          sourceToken{ InvalidExecToken };
    NodeScratch*       scratch{ nullptr };

    // AsyncIO 확장 — InvokeNode() 직전에 TaskExecutor가 설정.
    uint32_t           frameEpoch{ 0 };
    IIoHandleProvider* ioProvider{ nullptr };

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            frame != nullptr &&
            frame->IsValid() &&
            nodeId != InvalidExecNodeId &&
            scopeId != InvalidExecScopeId;
    }

    // ExecFn 내부에서 호출. IoHandle 획득 후 async 작업 제출, Suspend 반환.
    // nullptr 반환 시 ioProvider 미설정 또는 풀 고갈 → 호출부에서 Failed 처리.
    [[nodiscard]]
    IoHandle* RequestSuspend() noexcept
    {
        if (!ioProvider) return nullptr;
        return ioProvider->Acquire(nodeId, frameEpoch);
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
