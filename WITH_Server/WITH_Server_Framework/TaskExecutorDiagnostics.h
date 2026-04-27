#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionSourceTypes.h"

// ---------------------------------------------------------------------------
// TaskExecutorInvariantCounters
//
// SPEC-EXEC-QUEUE-001 §개선방향 §1 — frame 단위 invariant diagnostics.
// 이 값들은 단순 로그가 아니라 테스트가 읽을 수 있는 diagnostics로 노출된다.
// strict stress mode 에서는 AnyNonZero() == true 이면 테스트를 즉시 실패 처리한다.
// ---------------------------------------------------------------------------
struct TaskExecutorInvariantCounters
{
    // queue에서 꺼낸 entry의 Queued→Running / Queued→Cancel CAS가 실패한 횟수.
    // stale/duplicate queue entry가 discard된 정상 경로.
    uint32_t staleQueuedEntryDiscarded{ 0 };

    // successor의 remainingDeps 가 1→0 전이 후 NotReady→Ready CAS 가 실패한 횟수.
    uint32_t successorReadyTransitionSkipped{ 0 };

    // successor의 remainingDeps 가 1→0 전이 후 NotReady→Cancel CAS 가 실패한 횟수.
    uint32_t successorCancelTransitionSkipped{ 0 };

    // successor의 remainingDeps 가 이미 0인데 추가 predecessor completion이 관측된 횟수.
    uint32_t remainingDepsUnderflowAttempt{ 0 };

    // Ready→Queued CAS 가 실패한 횟수 (DispatchNode 내부).
    uint32_t dispatchReadyToQueuedFailed{ 0 };

    // Running→terminal CAS 가 실패한 횟수 (같은 node가 두 번 terminal 처리 시도).
    uint32_t duplicateCompletionAttempt{ 0 };

    // scopeRt->remainingNodes 가 이미 0인데 감소를 시도한 횟수.
    uint32_t scopeRemainingUnderflowAttempt{ 0 };

    // remainingSimulateNodes 가 이미 0인데 감소를 시도한 횟수.
    uint32_t simulateRemainingUnderflowAttempt{ 0 };

    [[nodiscard]] bool AnyNonZero() const noexcept
    {
        return staleQueuedEntryDiscarded != 0
            || successorReadyTransitionSkipped != 0
            || successorCancelTransitionSkipped != 0
            || remainingDepsUnderflowAttempt != 0
            || dispatchReadyToQueuedFailed != 0
            || duplicateCompletionAttempt != 0
            || scopeRemainingUnderflowAttempt != 0
            || simulateRemainingUnderflowAttempt != 0;
    }

    void Clear() noexcept
    {
        staleQueuedEntryDiscarded = 0;
        successorReadyTransitionSkipped = 0;
        successorCancelTransitionSkipped = 0;
        remainingDepsUnderflowAttempt = 0;
        dispatchReadyToQueuedFailed = 0;
        duplicateCompletionAttempt = 0;
        scopeRemainingUnderflowAttempt = 0;
        simulateRemainingUnderflowAttempt = 0;
    }
};

struct TaskExecutorDiagnosticsConfig
{
    bool enabled{ false };
    bool collectNodeTimings{ false };
    bool logFrameSummary{ false };
    bool writeCsv{ false };
    uint32_t sampleEveryNFrames{ 1 };
    std::string summaryCsvPath;
    std::string nodeCsvPath;
};

struct TaskExecutorNodeDiagnostic
{
    ExecNodeId nodeId{ InvalidExecNodeId };
    ExecScopeId scopeId{ InvalidExecScopeId };
    ExecToken sourceToken{ InvalidExecToken };
    ExecPhase phase{ ExecPhase::None };
    ExecNodeKind kind{ ExecNodeKind::None };
    uint32_t flags{ ExecNodeFlag_None };
    ExecCallResult result{ ExecCallResult::Failed };
    uint64_t threadHash{ 0 };
    uint64_t startNs{ 0 };
    uint64_t endNs{ 0 };
    uint64_t durationNs{ 0 };
    std::string debugName;
};

struct TaskExecutorFrameDiagnostics
{
    bool valid{ false };
    uint64_t frameOrdinal{ 0 };
    uint32_t workerCount{ 0 };
    uint32_t nodeCount{ 0 };
    uint32_t simulateNodeCount{ 0 };
    uint32_t scopeCount{ 0 };
    uint32_t edgeCount{ 0 };
    uint32_t executedNodes{ 0 };
    uint32_t maxConcurrency{ 0 };
    uint32_t distinctThreads{ 0 };
    uint64_t wallNs{ 0 };
    uint64_t sumNodeNs{ 0 };
    uint64_t minNodeNs{ 0 };
    uint64_t maxNodeNs{ 0 };
    uint64_t idealNs{ 0 };
    int64_t overheadVsIdealNs{ 0 };
    double workerEfficiency{ 0.0 };
    std::vector<TaskExecutorNodeDiagnostic> nodes;

    // SPEC-EXEC-QUEUE-001 invariant counters — always populated, regardless of
    // whether performance recording is enabled.
    TaskExecutorInvariantCounters invariants;

    void Clear() noexcept;
};

class TaskExecutorDiagnosticsRecorder final
{
public:
    TaskExecutorDiagnosticsRecorder();
    ~TaskExecutorDiagnosticsRecorder();

    TaskExecutorDiagnosticsRecorder(const TaskExecutorDiagnosticsRecorder&) = delete;
    TaskExecutorDiagnosticsRecorder& operator=(const TaskExecutorDiagnosticsRecorder&) = delete;

    void Configure(TaskExecutorDiagnosticsConfig config);

    void BeginFrame(
        const FrameTaskGraph& graph,
        uint32_t workerCount,
        uint64_t frameOrdinal);

    void EndFrame(
        const FrameTaskGraph& graph,
        const ExecutionSourceRegistry& sourceRegistry);

    void BeginNode(const ExecNodeRecord& node, ExecNodeId nodeId) noexcept;
    void EndNode(
        const ExecNodeRecord& node,
        ExecNodeId nodeId,
        ExecCallResult result) noexcept;

    // ---------------------------------------------------------------------------
    // Invariant counter record methods — thread-safe, always active.
    // SPEC-EXEC-QUEUE-001 §개선방향 §1
    // ---------------------------------------------------------------------------
    void RecordStaleQueuedEntry() noexcept;
    void RecordSuccessorReadyTransitionSkipped() noexcept;
    void RecordSuccessorCancelTransitionSkipped() noexcept;
    void RecordRemainingDepsUnderflow() noexcept;
    void RecordDispatchReadyToQueuedFailed() noexcept;
    void RecordDuplicateCompletion() noexcept;
    void RecordScopeRemainingUnderflow() noexcept;
    void RecordSimulateRemainingUnderflow() noexcept;

    [[nodiscard]]
    const TaskExecutorDiagnosticsConfig& GetConfig() const noexcept
    {
        return _config;
    }

    [[nodiscard]]
    const TaskExecutorFrameDiagnostics& GetLastFrame() const noexcept
    {
        return _lastFrame;
    }

private:
    struct NodeTimingSlot;
    static constexpr uint64_t kNsPerUs = 1000;
    static constexpr const char* kLogCategory = "ExecutorPerf";

    [[nodiscard]]
    static uint64_t NowNs() noexcept;
    [[nodiscard]]
    static double ToUs(uint64_t ns) noexcept;
    [[nodiscard]]
    static double ToUsSigned(int64_t ns) noexcept;
    [[nodiscard]]
    static const char* PhaseName(ExecPhase phase) noexcept;
    [[nodiscard]]
    static const char* KindName(ExecNodeKind kind) noexcept;
    [[nodiscard]]
    static const char* ResultName(ExecCallResult result) noexcept;

    static void UpdateAtomicMax(std::atomic<uint32_t>& target, uint32_t value) noexcept;

    void EnsureSlotCapacity(size_t nodeCount);
    void ResetSlots(size_t nodeCount) noexcept;
    void BuildFrameSnapshot(
        const FrameTaskGraph& graph,
        const ExecutionSourceRegistry& sourceRegistry,
        uint64_t frameEndNs);
    void EmitFrameSummary() const;
    void WriteCsvFiles();
    void WriteSummaryCsv();
    void WriteNodeCsv();

private:
    TaskExecutorDiagnosticsConfig _config{};
    TaskExecutorFrameDiagnostics _lastFrame{};
    std::unique_ptr<NodeTimingSlot[]> _slots;
    size_t _slotCapacity{ 0 };
    size_t _activeSlotCount{ 0 };
    uint64_t _frameStartNs{ 0 };
    uint32_t _workerCount{ 0 };
    uint64_t _frameOrdinal{ 0 };
    bool _recording{ false };
    bool _summaryHeaderWritten{ false };
    bool _nodeHeaderWritten{ false };
    std::atomic<uint32_t> _activeNodes{ 0 };
    std::atomic<uint32_t> _maxActiveNodes{ 0 };
    std::atomic<uint32_t> _executedNodes{ 0 };

    // Invariant counters — always tracked regardless of _recording.
    std::atomic<uint32_t> _staleQueuedEntryDiscarded{ 0 };
    std::atomic<uint32_t> _successorReadyTransitionSkipped{ 0 };
    std::atomic<uint32_t> _successorCancelTransitionSkipped{ 0 };
    std::atomic<uint32_t> _remainingDepsUnderflowAttempt{ 0 };
    std::atomic<uint32_t> _dispatchReadyToQueuedFailed{ 0 };
    std::atomic<uint32_t> _duplicateCompletionAttempt{ 0 };
    std::atomic<uint32_t> _scopeRemainingUnderflowAttempt{ 0 };
    std::atomic<uint32_t> _simulateRemainingUnderflowAttempt{ 0 };
};
