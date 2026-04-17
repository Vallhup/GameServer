#pragma once

#include <cstdint>
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

using ExecFn = ExecCallResult(*)(struct NodeExecContext&);

constexpr bool IsTerminalNodeState(ExecNodeState state) noexcept
{
    switch (state) {
    case ExecNodeState::Succeeded:
    case ExecNodeState::Failed:
    case ExecNodeState::Canceled:
    case ExecNodeState::Skipped:
        return true;
    default:
        return false;
    }
}

constexpr bool IsExecutableScopePhase(ExecScopePhase phase) noexcept
{
    return phase == ExecScopePhase::Open;
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
