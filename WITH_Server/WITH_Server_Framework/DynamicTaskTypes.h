#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "SystemMeta.h"

struct NodeExecContext;
using ExecFn = ExecCallResult(*)(NodeExecContext&);

// Forward declaration — ExecutionSourceTypes.h에서 완전 정의된다.
struct ExecutionSourceDesc;
class ExecutionSourceRegistry;

// ---------------------------------------------------------------------------
// DynamicTaskTypeId
// ---------------------------------------------------------------------------
using DynamicTaskTypeId = uint32_t;
constexpr DynamicTaskTypeId InvalidDynamicTaskTypeId = 0;

// ---------------------------------------------------------------------------
// DynamicTaskTypeDesc
//
// 재사용 가능한 Dynamic Task '타입'의 메타데이터 — 인스턴스가 아니다.
// DynamicTaskTypeRegistry::Register() 호출 시 ExecutionSourceDesc로 변환되어
// ExecutionSourceRegistry에도 동시 등록된다.
//
// 등록 전: typeId == InvalidDynamicTaskTypeId, sourceToken == InvalidExecToken
// 등록 후: 두 필드 모두 유효한 값으로 채워진다.
// ---------------------------------------------------------------------------
struct DynamicTaskTypeDesc
{
    DynamicTaskTypeId typeId{ InvalidDynamicTaskTypeId };   // 등록 후 채워진다
    ExecToken         sourceToken{ InvalidExecToken };      // 등록 후 채워진다

    ExecPhase      defaultPhase{ ExecPhase::Simulate };
    ExecLane       defaultLane{ ExecLane::Parallel };
    uint32_t       flags{ ExecNodeFlag_None };

    // 이 타입의 모든 인스턴스에 적용되는 resource access 선언.
    // DynamicTaskTypeRegistry가 소유 (수명 = 레지스트리).
    std::vector<AccessSpec> accesses;

    ExecFn         dispatchFn{ nullptr };
    SchedulingHint schedulingHint{};
    std::string    debugName;

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return !debugName.empty() && dispatchFn != nullptr;
    }
};

// ---------------------------------------------------------------------------
// DynamicTaskRequest
//
// Gameplay / Network / DB 코드가 프레임 실행 중 다음 프레임을 위해 발행하는 요청.
// 즉시 실행되지 않으며 DynamicTaskPendingQueue에 누적된다.
// 다음 프레임 그래프 빌드 직전에 동결(freeze)되어 DynamicTaskFrozenBatch가 된다.
// ---------------------------------------------------------------------------
struct DynamicTaskRequest
{
    DynamicTaskTypeId typeId{ InvalidDynamicTaskTypeId };
    ExecScopeId       scopeId{ InvalidExecScopeId };

    // payload 식별자: 해석은 dispatch 함수가 담당한다.
    // Phase 1에서는 game-side handle index 등 uint64 key로 충분하다.
    uint64_t payloadKey{ 0 };

    // 우선순위 편향 — 동일 Phase 내 실행 순서 조정용.
    int32_t  priorityBias{ 0 };

    // 요청이 발행된 프레임 인덱스 (디버그 / 진단 / 결정론적 정렬 tie-break).
    uint64_t requestFrameIndex{ 0 };

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return typeId != InvalidDynamicTaskTypeId
            && scopeId != InvalidExecScopeId;
    }
};

// ---------------------------------------------------------------------------
// DynamicTaskFrozenBatch
//
// 한 프레임 그래프 빌드에 사용되는 동결 요청 집합.
// 그래프 빌드 전에 결정론적 정렬이 완료된 상태여야 한다.
// 정렬 기준: (scopeId ASC, priorityBias DESC, requestFrameIndex ASC)
// ---------------------------------------------------------------------------
struct DynamicTaskFrozenBatch
{
    std::vector<DynamicTaskRequest> requests;

    [[nodiscard]]
    bool IsEmpty() const noexcept { return requests.empty(); }

    // 결정론적 정렬을 수행한다. Drain 후, 그래프 빌드 전에 반드시 호출해야 한다.
    void Sort()
    {
        std::sort(requests.begin(), requests.end(),
            [](const DynamicTaskRequest& a, const DynamicTaskRequest& b)
            {
                if (a.scopeId != b.scopeId)
                    return a.scopeId < b.scopeId;
                if (a.priorityBias != b.priorityBias)
                    return a.priorityBias > b.priorityBias; // 높을수록 먼저
                return a.requestFrameIndex < b.requestFrameIndex;
            });
    }

    void Clear() { requests.clear(); }
};

// ---------------------------------------------------------------------------
// DynamicTaskInstance
//
// 그래프 빌드 중 DynamicTaskFrozenBatch 항목으로부터 생성되는 프레임 인스턴스.
// DynamicTaskFrameTable이 소유하며 프레임 실행 내내 불변이다.
// ---------------------------------------------------------------------------
struct DynamicTaskInstance
{
    DynamicTaskTypeId typeId{ InvalidDynamicTaskTypeId };
    ExecScopeId       scopeId{ InvalidExecScopeId };
    ExecNodeId        graphNodeId{ InvalidExecNodeId };
    uint64_t          payloadKey{ 0 };
};

// ---------------------------------------------------------------------------
// DynamicTaskFrameTable
//
// 한 프레임의 동결된 DynamicTask 인스턴스 테이블.
// 그래프 빌드 단계에서 AddInstance()로 채워지며, 프레임 실행 중에는 불변이다.
// Executor는 nodeId로 인스턴스를 조회하여 payload를 resolve한다.
// ---------------------------------------------------------------------------
class DynamicTaskFrameTable
{
public:
    // 그래프 빌드 단계에서 호출 — 인스턴스를 추가하고 인덱스를 반환한다.
    uint32_t AddInstance(DynamicTaskInstance instance)
    {
        const uint32_t idx = static_cast<uint32_t>(_instances.size());
        _nodeIdToIndex[instance.graphNodeId] = idx;
        _instances.push_back(std::move(instance));
        return idx;
    }

    // Executor dispatch 단계에서 호출 — nodeId로 인스턴스를 조회한다.
    [[nodiscard]]
    const DynamicTaskInstance* FindByNodeId(ExecNodeId nodeId) const noexcept
    {
        const auto it = _nodeIdToIndex.find(nodeId);
        if (it == _nodeIdToIndex.end()) return nullptr;
        return &_instances[it->second];
    }

    [[nodiscard]]
    uint32_t InstanceCount() const noexcept
    {
        return static_cast<uint32_t>(_instances.size());
    }

    [[nodiscard]]
    bool IsEmpty() const noexcept { return _instances.empty(); }

    void Clear()
    {
        _instances.clear();
        _nodeIdToIndex.clear();
    }

private:
    std::vector<DynamicTaskInstance>          _instances;
    std::unordered_map<ExecNodeId, uint32_t>  _nodeIdToIndex;
};

// ---------------------------------------------------------------------------
// DynamicTaskPendingQueue
//
// 프레임 실행 중 시스템/네트워크 코드가 다음 프레임을 위해 Push하는 큐.
// Push는 병렬 시스템에서 호출되므로 thread-safe.
// DrainInto는 프레임 경계(단일 스레드)에서만 호출된다.
//
// 소유권: WorldRuntime이 per-scope 인스턴스를 보유한다.
// ---------------------------------------------------------------------------
class DynamicTaskPendingQueue
{
public:
    // thread-safe. 병렬 System 또는 Asio 콜백에서 호출 가능하다.
    void Push(DynamicTaskRequest request)
    {
        std::lock_guard lock{ _mutex };
        _pending.push_back(std::move(request));
    }

    // 프레임 경계(단일 스레드)에서 호출한다.
    // pending 전체를 out에 이동 추가하고 큐를 비운다.
    void DrainInto(std::vector<DynamicTaskRequest>& out)
    {
        std::lock_guard lock{ _mutex };
        out.insert(
            out.end(),
            std::make_move_iterator(_pending.begin()),
            std::make_move_iterator(_pending.end()));
        _pending.clear();
    }

    [[nodiscard]]
    uint32_t PendingCount() const noexcept
    {
        std::lock_guard lock{ _mutex };
        return static_cast<uint32_t>(_pending.size());
    }

private:
    mutable std::mutex              _mutex;
    std::vector<DynamicTaskRequest> _pending;
};

// ---------------------------------------------------------------------------
// DynamicTaskTypeRegistry
//
// DynamicTaskTypeDesc의 등록 및 조회.
// ExecutionSourceRegistry와 짝으로 사용된다.
// Register()는 ExecutionSourceRegistry에도 대응하는 ExecutionSourceDesc를 등록한다.
// ---------------------------------------------------------------------------
class DynamicTaskTypeRegistry
{
public:
    // desc를 등록하고 typeId / sourceToken을 채워서 반환한다.
    // 실패 시 InvalidDynamicTaskTypeId를 반환한다.
    [[nodiscard]]
    DynamicTaskTypeId Register(
        DynamicTaskTypeDesc desc,
        ExecutionSourceRegistry& sourceRegistry);

    [[nodiscard]]
    const DynamicTaskTypeDesc* TryGet(DynamicTaskTypeId typeId) const noexcept
    {
        for (const DynamicTaskTypeDesc& d : _types)
        {
            if (d.typeId == typeId)
                return &d;
        }
        return nullptr;
    }

    [[nodiscard]]
    const DynamicTaskTypeDesc* TryGetByToken(ExecToken token) const noexcept
    {
        for (const DynamicTaskTypeDesc& d : _types)
        {
            if (d.sourceToken == token)
                return &d;
        }
        return nullptr;
    }

    [[nodiscard]]
    uint32_t TypeCount() const noexcept
    {
        return static_cast<uint32_t>(_types.size());
    }

    void Clear()
    {
        _types.clear();
        _nextId.store(1, std::memory_order_relaxed);
    }

private:
    // deque: push_back 시 기존 원소 포인터 안정 → accesses span이 dangling되지 않는다.
    std::deque<DynamicTaskTypeDesc>  _types;
    std::atomic<DynamicTaskTypeId>   _nextId{ 1 };
};
