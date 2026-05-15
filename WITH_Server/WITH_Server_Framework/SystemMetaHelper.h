#pragma once

#include "SystemMeta.h"

// ---------------------------------------------------------------------------
// ResourceId 생성 헬퍼 (컴파일 타임) [v3 갱신]
//
// 내부 구현이 uint64 MakeResourceId로 전환되었으나 시그니처는 유지된다.
// ---------------------------------------------------------------------------

// 태그 전용 마커 타입 (DirtyTracker/CommandBuffer는 인스턴스가 없으므로 타입만 사용)
struct DirtyTrackerResourceTag
{};
struct CommandBufferResourceTag
{};

template<typename T>
[[nodiscard]] consteval ResourceId ComponentRes() noexcept
{
    return MakeResourceId<ResourceKinds::Component, T>();
}

template<typename T>
[[nodiscard]] consteval ResourceId EventRes() noexcept
{
    return MakeResourceId<ResourceKinds::EventQueue, T>();
}

template<typename T>
[[nodiscard]] consteval ResourceId ExternalRes() noexcept
{
    return MakeResourceId<ResourceKinds::External, T>();
}

[[nodiscard]] consteval ResourceId DirtyTrackerRes() noexcept
{
    return MakeResourceId<ResourceKinds::DirtyTracker, DirtyTrackerResourceTag>();
}

[[nodiscard]] consteval ResourceId CommandBufferRes() noexcept
{
    return MakeResourceId<ResourceKinds::CommandBuffer, CommandBufferResourceTag>();
}

// ---------------------------------------------------------------------------
// AccessSpec 생성 헬퍼 [v3 갱신]
//
// StructuralEffect 파라미터 제거. archetype 변경 선언은 ExecMeta 수준의
// canMutateArchetype / canCreateDestroyEntity bool 플래그로 이전한다.
// ---------------------------------------------------------------------------
[[nodiscard]] inline AccessSpec ReadSnapshot(ResourceId resource) noexcept
{
    return { resource, AccessMode::Read, Visibility::Snapshot };
}

[[nodiscard]] inline AccessSpec ReadImmediate(ResourceId resource) noexcept
{
    return { resource, AccessMode::Read, Visibility::Immediate };
}

[[nodiscard]] inline AccessSpec WriteImmediate(ResourceId resource) noexcept
{
    return { resource, AccessMode::Write, Visibility::Immediate };
}

[[nodiscard]] inline AccessSpec WriteDeferred(ResourceId resource) noexcept
{
    return { resource, AccessMode::Write, Visibility::Deferred };
}

[[nodiscard]] inline AccessSpec EmitDeferred(ResourceId resource) noexcept
{
    return { resource, AccessMode::Emit, Visibility::Deferred };
}

[[nodiscard]] inline AccessSpec ConsumeDeferred(ResourceId resource) noexcept
{
    return { resource, AccessMode::Consume, Visibility::Deferred };
}

// ConsumeCommit -> {CommandBuffer, Consume, Deferred} 대체 [v3 갱신]
// resource는 CommandBufferRes()로 생성해야 한다.
// 이름을 유지하여 기존 코드 호환성 보장, 내부 구현만 변경.
[[nodiscard]] inline AccessSpec ConsumeCommit(ResourceId resource) noexcept
{
    return { resource, AccessMode::Consume, Visibility::Deferred };
}

// ---------------------------------------------------------------------------
// ExecTag / SystemTag 생성 헬퍼 [v3 갱신: SysTag -> Tag, SysTag alias 유지]
// ---------------------------------------------------------------------------
template<typename T>
[[nodiscard]] consteval ExecTag MakeExecTag() noexcept
{
    return TypeHash<T>();
}

template<typename T>
[[nodiscard]] consteval ExecTag Tag() noexcept
{
    return MakeExecTag<T>();
}

// Backward compat alias -- 기존 게임 코드의 SysTag<T>() 참조를 유지한다.
template<typename T>
[[nodiscard]] consteval ExecTag SysTag() noexcept
{
    return Tag<T>();
}
