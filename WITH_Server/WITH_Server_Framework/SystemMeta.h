#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "ExecutionCoreTypes.h" // TypeHash<T>

// ---------------------------------------------------------------------------
// ResourceKind -- 확장 가능한 uint8 [v3 갱신]
//
// 기존 닫힌 enum class에서 using ResourceKind = uint8_t로 전환한다.
// 구간 분리:
//   0-4  : 내장 (BuiltinHasConflict 빠른 경로 대상)
//   5-15 : 예약
//   16-255: 사용자 정의 (ConflictResolver 등록 필요)
// ---------------------------------------------------------------------------
using ResourceKind = uint8_t;

namespace ResourceKinds
{
    constexpr ResourceKind Component      = 0;
    constexpr ResourceKind EventQueue     = 1;
    constexpr ResourceKind DirtyTracker   = 2;
    constexpr ResourceKind CommandBuffer  = 3;
    constexpr ResourceKind External       = 4;

    constexpr ResourceKind kUserDefinedBase = 16;
}

// ---------------------------------------------------------------------------
// ResourceId -- uint64 균일 키 [v3 갱신]
//
// upper 8bit : ResourceKind
// lower 56bit: TypeHash(컴파일 타임) 또는 Registry 할당 id(런타임)
// ---------------------------------------------------------------------------
using ResourceId = uint64_t;

[[nodiscard]] constexpr ResourceKind KindOf(ResourceId id) noexcept
{
    return static_cast<ResourceKind>(id >> 56);
}

[[nodiscard]] constexpr uint64_t IdOf(ResourceId id) noexcept
{
    return id & 0x00FFFFFFFFFFFFFF;
}

template<ResourceKind Kind, typename T>
[[nodiscard]] consteval ResourceId MakeResourceId() noexcept
{
    return (static_cast<uint64_t>(Kind) << 56)
        | (TypeHash<T>() & 0x00FFFFFFFFFFFFFF);
}

// ---------------------------------------------------------------------------
// AccessMode (변경 없음)
// ---------------------------------------------------------------------------
enum class AccessMode : uint8_t
{
    Read,
    Write,
    Emit,
    Consume
};

// ---------------------------------------------------------------------------
// Visibility -- Commit 제거 [v3 갱신]
//
// Visibility::Commit 제거 이유: CommandBuffer는 ResourceKind로 이미 식별된다.
// Commit Phase 타이밍은 Phase Rank가 담당하므로 중복 인코딩 불필요.
// 기존 ConsumeCommit DSL은 {CommandBuffer, Consume, Deferred}로 대체된다.
// ---------------------------------------------------------------------------
enum class Visibility : uint8_t
{
    Snapshot,
    Immediate,
    Deferred,
};

// ---------------------------------------------------------------------------
// AccessSpec -- 3개 필드 [v3 갱신]
//
// StructuralEffect 제거: archetype 무효화 정보는 HasConflict()에서 미사용.
// ExecMeta 수준의 canMutateArchetype / canCreateDestroyEntity bool 플래그로 격상.
// ---------------------------------------------------------------------------
struct AccessSpec
{
    ResourceId resource;    // uint64: kind(8) | id(56)
    AccessMode mode;        // Read, Write, Emit, Consume
    Visibility visibility;  // Snapshot, Immediate, Deferred

    bool operator==(const AccessSpec&) const = default;
};

// ---------------------------------------------------------------------------
// ExecTag -- 실행 단위 식별자 [v3 갱신: SystemTag -> ExecTag]
// ---------------------------------------------------------------------------
using ExecTag = uint64_t;
constexpr ExecTag InvalidExecTag = 0;

// Backward compat: 기존 게임 코드의 SystemTag 참조를 유지한다.
using SystemTag = ExecTag;

// ---------------------------------------------------------------------------
// SchedulingHint -- 선택적 스케줄링 품질 힌트 [spec 3.8절]
//
// 의존성 그래프의 정확성(Correctness)과 무관하다.
// 동일 Phase 내에서 여러 노드가 동시에 Ready 상태일 때 실행 순서를 조정하는 힌트다.
// ---------------------------------------------------------------------------
struct SchedulingHint
{
    // 양수: 조기 스케줄 선호 (critical path 앞쪽 배치).
    // 음수: 지연 허용 (non-critical, tail 노드 등 후순위 처리).
    // 0: 기본값 -- 우선순위 편향 없음.
    int32_t priorityBias{ 0 };

    // true: 가능하면 메인 스레드 실행을 선호한다.
    // ExecNodeFlag_MainThreadOnly와 달리 강제 조건이 아닌 힌트이다.
    bool preferMainThread{ false };
};

// ---------------------------------------------------------------------------
// ExecMeta -- 통합 실행 메타데이터 [v3 갱신: SystemMeta -> ExecMeta]
// ---------------------------------------------------------------------------
struct ExecMeta
{
    ExecTag          tag;
    std::string_view name;

    std::span<const AccessSpec> accesses;
    std::span<const ExecTag>    runsBefore;
    std::span<const ExecTag>    runsAfter;

    // ECS 구조 변경 선언 -- HasConflict()에 관여하지 않음. ECS 빌더 전용.
    bool canMutateArchetype    { false }; // AddComponent / RemoveComponent 포함 여부
    bool canCreateDestroyEntity{ false }; // CreateEntity  / DestroyEntity  포함 여부

    // 스케줄링 품질 힌트 -- 의존성 그래프 정확성에 영향을 주지 않는다.
    SchedulingHint schedulingHint{};
};

// Backward compat: 기존 게임 코드의 SystemMeta 참조를 유지한다.
using SystemMeta = ExecMeta;
