#pragma once

#include <cstdint>
#include <unordered_map>

#include "SystemMeta.h" // AccessSpec, AccessMode, Visibility, ResourceKind, ResourceKinds, KindOf

// ---------------------------------------------------------------------------
// BuiltinHasConflict -- 5-rule 알고리즘 (빌트인 ResourceKind 전용)
//
// 두 AcceseSpec이 동일 Resource를 참조할 때 실행 순서를 강제해야 하는지 판단.
//
//   Rule 1: 어느 한쪽이 Snapshot   → false  (스냅샷은 항상 안전)
//   Rule 2: 둘 다 Read             → false  (동시 읽기 가능)
//   Rule 3: Deferred Write vs Read → false  (Deferred는 Commit 이후 적용)
//   Rule 4: 둘 다 Emit             → false  (다중 이벤트 생산자 허용)
//   Rule 5: 그 외 전부             → true   (충돌)
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool BuiltinHasConflict(
    const AccessSpec& a,
    const AccessSpec& b) noexcept
{
    // Rule 1 — Snapshot은 항상 안전
    if (a.visibility == Visibility::Snapshot ||
        b.visibility == Visibility::Snapshot)
    {
        return false;
    }

    // Rule 2 — 양쪽 모두 Read
    if (a.mode == AccessMode::Read && b.mode == AccessMode::Read)
        return false;

    // Rule 3 — Deferred Write/Emit/Consume 한쪽 + 반대편 Read
    //          Deferred 부수효과는 Commit Phase 이후에 반영되므로 충돌하지 않는다.
    const bool aDeferred = (a.visibility == Visibility::Deferred);
    const bool bDeferred = (b.visibility == Visibility::Deferred);

    if ((aDeferred && b.mode == AccessMode::Read) ||
        (bDeferred && a.mode == AccessMode::Read))
    {
        return false;
    }

    // Rule 4 — 양쪽 모두 Emit (다중 생산자 이벤트 큐)
    if (a.mode == AccessMode::Emit && b.mode == AccessMode::Emit)
        return false;

    // Rule 5 — 위 모든 안전 규칙을 통과하지 못한 경우 → 충돌
    return true;
}

// ---------------------------------------------------------------------------
// ConflictFn -- 사용자 정의 ResourceKind의 충돌 판단 콜백 시그니처.
//
// 호출 전제: a.resource == b.resource (동일 리소스에 대한 두 접근)
// ---------------------------------------------------------------------------
using ConflictFn = bool (*)(const AccessSpec& a, const AccessSpec& b) noexcept;

// ---------------------------------------------------------------------------
// ConflictRegistry -- 사용자 정의 ResourceKind 충돌 정책 등록소.
//
// ResourceKinds::kUserDefinedBase (16) 이상의 kind에 ConflictFn을 등록한다.
// 등록되지 않은 사용자 정의 kind는 보수적 폴백(Read+Read 제외 전부 충돌)을 사용.
// ---------------------------------------------------------------------------
class ConflictRegistry {
public:
    // kind는 반드시 ResourceKinds::kUserDefinedBase(16) 이상이어야 한다.
    // 빌트인 kind(0-15)에 대한 등록은 무시한다.
    void Register(ResourceKind kind, ConflictFn fn)
    {
        if (kind >= ResourceKinds::kUserDefinedBase && fn != nullptr)
            _customFns[kind] = fn;
    }

    [[nodiscard]]
    ConflictFn TryGet(ResourceKind kind) const noexcept
    {
        const auto it = _customFns.find(kind);
        if (it == _customFns.end())
            return nullptr;
        return it->second;
    }

    void Clear() noexcept
    {
        _customFns.clear();
    }

private:
    std::unordered_map<ResourceKind, ConflictFn> _customFns;
};

// ---------------------------------------------------------------------------
// HasConflict -- 통합 진입점.
//
// 두 AccessSpec이 동일 Resource를 참조하는 경우에만 의미가 있다.
// 호출 전에 a.resource == b.resource 를 보장해야 한다.
//
// conflictRegistry == nullptr 일 때는 BuiltinHasConflict만 사용.
// 사용자 정의 kind이면서 등록된 ConflictFn이 없으면 보수적 폴백을 사용한다.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool HasConflict(
    const AccessSpec& a,
    const AccessSpec& b,
    const ConflictRegistry* registry) noexcept
{
    const ResourceKind kind = KindOf(a.resource);

    if (kind < ResourceKinds::kUserDefinedBase)
    {
        // 빌트인 kind — 5-rule 알고리즘
        return BuiltinHasConflict(a, b);
    }

    // 사용자 정의 kind — 등록된 함수 우선, 없으면 보수적 폴백
    if (registry != nullptr)
    {
        const ConflictFn fn = registry->TryGet(kind);
        if (fn != nullptr)
            return fn(a, b);
    }

    // 폴백: Read+Read 이외는 충돌로 간주
    return !(a.mode == AccessMode::Read && b.mode == AccessMode::Read);
}

#if defined(_DEBUG)
// ---------------------------------------------------------------------------
// IsValidAccessSpec -- AccessSpec 유효 조합 검사 [spec 3.5절, Debug 전용]
//
// AccessMode × Visibility = 4 × 3 = 12가지 조합 중 유효한 것은 11종이다.
// 유일한 무효 조합: Write + Snapshot
//
//   Read    + Snapshot  ✓    Read    + Immediate  ✓    Read    + Deferred  ✓
//   Write   + Snapshot  ✗    Write   + Immediate  ✓    Write   + Deferred  ✓
//   Emit    + Snapshot  ✓    Emit    + Immediate  ✓    Emit    + Deferred  ✓
//   Consume + Snapshot  ✓    Consume + Immediate  ✓    Consume + Deferred  ✓
//
// Write + Snapshot 이 무효인 이유:
//   Snapshot은 읽기 전용 뷰이다. Write 의도와의 결합은 의미론적으로 모순이며,
//   BuiltinHasConflict Rule 1(Snapshot → 항상 안전) 의 전제를 위반할 수 있다.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool IsValidAccessSpec(const AccessSpec& spec) noexcept
{
    if (spec.mode == AccessMode::Write && spec.visibility == Visibility::Snapshot)
        return false;

    return true;
}
#endif

// ---------------------------------------------------------------------------
// HasAnyConflict -- 두 AccessSpec 집합 간에 충돌 쌍이 하나라도 있는지 검사.
//
// 동일 resource를 참조하는 쌍에 대해서만 HasConflict를 호출한다.
// O(|a| * |b|) — 시스템 간 접근 목록은 수십 개 수준이므로 충분하다.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool HasAnyConflict(
    std::span<const AccessSpec> a,
    std::span<const AccessSpec> b,
    const ConflictRegistry* registry) noexcept
{
    for (const AccessSpec& specA : a)
    {
        for (const AccessSpec& specB : b)
        {
            if (specA.resource != specB.resource)
                continue;

            if (HasConflict(specA, specB, registry))
                return true;
        }
    }
    return false;
}
