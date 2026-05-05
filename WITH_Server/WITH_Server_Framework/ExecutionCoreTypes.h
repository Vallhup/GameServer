#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

using ExecNodeId = uint32_t;
using ExecScopeId = uint32_t;
using ExecToken = uint32_t;

constexpr ExecNodeId  InvalidExecNodeId = static_cast<ExecNodeId>(-1);
constexpr ExecScopeId InvalidExecScopeId = static_cast<ExecScopeId>(-1);
constexpr ExecToken   InvalidExecToken = 0;

enum class ExecPhase : uint8_t
{
    None,
    Simulate,
    Commit,
    LifecycleFlush,
    Reconcile,
};

enum class ExecLane : uint8_t
{
    None,
    Parallel,
    Serial,
    Main,
};

enum class ExecNodeKind : uint8_t
{
    None,

    // simulate 계열
    StaticSystem,
    DynamicTask,

    // commit 계열
    StructuralApply,
    DeferredStateApply,
    PostCommitFinalize,

    // post-frame 계열
    LifecycleFlush,
    Reconcile,
};

enum class ExecNodeState : uint8_t
{
    NotReady,
    Ready,
    Queued,
    Running,
    Suspended,  // 아웃바운드 IO 대기 중. Terminal 아님 — 같은 프레임 내 재개 가능.

    Succeeded,
    Failed,
    Canceled,
    Skipped,
};

enum class ExecScopePhase : uint8_t
{
    Open,
    CancelRequested,
    Draining,
    Closed,
};

enum class ExecCallResult : uint8_t
{
    Success,
    Failed,
    Suspend,    // IO 대기. 같은 프레임 내 CompletionQueue 경유 재개 시도.
};

enum class ExecFailureClass : uint8_t
{
    None,

    NodeFailure,
    ScopeCanceled,

    BuildInvalid,
    ExecutorInvariant,
};

enum class BuildDiagnosticSeverity : uint8_t
{
    Info,
    Warning,
    Error,
};

enum ExecNodeFlags : uint32_t
{
    ExecNodeFlag_None = 0,
    ExecNodeFlag_AllowSkipOnCancel = 1u << 0,
    ExecNodeFlag_NoThrow = 1u << 1,
    ExecNodeFlag_MainThreadOnly = 1u << 2,
};

enum ExecScopeFlags : uint8_t
{
    ExecScopeFlag_None = 0,
    ExecScopeFlag_HasFailure = 1u << 0,
    ExecScopeFlag_WasCanceled = 1u << 1,
};

// ExecFn의 정식 선언은 NodeExecContext 완전 정의가 있는
// ExecutionContextTypes.h(137행)에서 한다. 이 전방 선언 버전은 제거하여
// 두 헤더를 같은 TU에서 include할 때 발생하는 중복 선언을 방지한다.

constexpr bool IsTerminalNodeState(ExecNodeState state) noexcept
{
    switch (state) {
    case ExecNodeState::Succeeded:
    case ExecNodeState::Failed:
    case ExecNodeState::Canceled:
    case ExecNodeState::Skipped:
        return true;
    case ExecNodeState::Suspended:  // IO 대기 중 — Terminal 아님
        return false;
    default:
        return false;
    }
}

// Open: 정상 실행 가능 상태.
// Draining: Suspend된 비동기 노드가 잔존하는 과도기 — 신규 노드는 여전히 실행 가능.
//           (Phase 1에서는 Cancel Draining 시나리오만 사용되며 Draining은 순간 경유 상태)
constexpr bool IsExecutableScopePhase(ExecScopePhase phase) noexcept
{
    return phase == ExecScopePhase::Open
        || phase == ExecScopePhase::Draining;
}

constexpr bool IsSerialPhase(ExecPhase phase) noexcept
{
    switch (phase) {
    case ExecPhase::Commit:
    case ExecPhase::LifecycleFlush:
    case ExecPhase::Reconcile:
        return true;
    default:
        return false;
    }
}

constexpr bool IsPostSimulatePhase(ExecPhase phase) noexcept
{
    return phase != ExecPhase::Simulate;
}

template <typename TEnum>
constexpr auto ToUnderlying(TEnum value) noexcept
    -> std::underlying_type_t<TEnum>
{
    static_assert(std::is_enum_v<TEnum>);
    return static_cast<std::underlying_type_t<TEnum>>(value);
}

constexpr bool HasAnyNodeFlag(uint32_t flags, ExecNodeFlags test) noexcept
{
    return (flags & static_cast<uint32_t>(test)) != 0;
}

constexpr bool HasAnyScopeFlag(uint8_t flags, ExecScopeFlags test) noexcept
{
    return (flags & static_cast<uint8_t>(test)) != 0;
}

constexpr void SetNodeFlag(uint32_t& flags, ExecNodeFlags flag) noexcept
{
    flags |= static_cast<uint32_t>(flag);
}

constexpr void SetScopeFlag(uint8_t& flags, ExecScopeFlags flag) noexcept
{
    flags |= static_cast<uint8_t>(flag);
}

// ---------------------------------------------------------------------------
// TypeHash<T>
//
// 컴파일 타임 타입 식별자 해시 (FNV-1a 64-bit over __FUNCSIG__ / __PRETTY_FUNCTION__).
// ResourceId uint64 키 체계에서 type_index 대신 사용한다.
// 동일 타입에 대해 동일 빌드 내에서 항상 동일한 값을 반환하며, 충돌 가능성은
// FNV-1a 64-bit 수준(< 2^-64)이다.
// ---------------------------------------------------------------------------
template<typename T>
consteval uint64_t TypeHash() noexcept
{
#if defined(_MSC_VER)
    constexpr std::string_view signature{ __FUNCSIG__ };
#elif defined(__GNUC__) || defined(__clang__)
    constexpr std::string_view signature{ __PRETTY_FUNCTION__ };
#else
    static_assert(false, "TypeHash: unsupported compiler");
#endif

    constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
    constexpr uint64_t kFnvPrime       = 1099511628211ULL;

    uint64_t hash = kFnvOffsetBasis;
    for (const char c : signature)
    {
        hash ^= static_cast<uint8_t>(c);
        hash *= kFnvPrime;
    }
    return hash;
}
