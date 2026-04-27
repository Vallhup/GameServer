#include "pch.h"
#include "TaskExecutorDiagnostics.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <thread>
#include <unordered_set>
#include <utility>

#include "FrameworkLog.h"

struct TaskExecutorDiagnosticsRecorder::NodeTimingSlot
{
    std::atomic<int64_t> startNs{ -1 };
    std::atomic<int64_t> endNs{ -1 };
    std::atomic<uint64_t> threadHash{ 0 };
    std::atomic<uint32_t> result{ static_cast<uint32_t>(ExecCallResult::Failed) };
};

void TaskExecutorFrameDiagnostics::Clear() noexcept
{
    valid = false;
    frameOrdinal = 0;
    workerCount = 0;
    nodeCount = 0;
    simulateNodeCount = 0;
    scopeCount = 0;
    edgeCount = 0;
    executedNodes = 0;
    maxConcurrency = 0;
    distinctThreads = 0;
    wallNs = 0;
    sumNodeNs = 0;
    minNodeNs = 0;
    maxNodeNs = 0;
    idealNs = 0;
    overheadVsIdealNs = 0;
    workerEfficiency = 0.0;
    nodes.clear();
    invariants.Clear();
}

TaskExecutorDiagnosticsRecorder::TaskExecutorDiagnosticsRecorder() = default;
TaskExecutorDiagnosticsRecorder::~TaskExecutorDiagnosticsRecorder() = default;

// ---------------------------------------------------------------------------
// Invariant counter record methods
// ---------------------------------------------------------------------------

void TaskExecutorDiagnosticsRecorder::RecordStaleQueuedEntry() noexcept
{
    _staleQueuedEntryDiscarded.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordSuccessorReadyTransitionSkipped() noexcept
{
    _successorReadyTransitionSkipped.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordSuccessorCancelTransitionSkipped() noexcept
{
    _successorCancelTransitionSkipped.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordRemainingDepsUnderflow() noexcept
{
    _remainingDepsUnderflowAttempt.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordDispatchReadyToQueuedFailed() noexcept
{
    _dispatchReadyToQueuedFailed.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordDuplicateCompletion() noexcept
{
    _duplicateCompletionAttempt.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordScopeRemainingUnderflow() noexcept
{
    _scopeRemainingUnderflowAttempt.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::RecordSimulateRemainingUnderflow() noexcept
{
    _simulateRemainingUnderflowAttempt.fetch_add(1, std::memory_order_relaxed);
}

void TaskExecutorDiagnosticsRecorder::Configure(TaskExecutorDiagnosticsConfig config)
{
    if (config.sampleEveryNFrames == 0)
    {
        config.sampleEveryNFrames = 1;
    }

    _config = std::move(config);
    _lastFrame.Clear();
    _summaryHeaderWritten = false;
    _nodeHeaderWritten = false;
}

void TaskExecutorDiagnosticsRecorder::BeginFrame(
    const FrameTaskGraph& graph,
    uint32_t workerCount,
    uint64_t frameOrdinal)
{
    _frameOrdinal = frameOrdinal;
    _workerCount = workerCount;
    _recording =
        _config.enabled &&
        (_config.sampleEveryNFrames == 0 ||
         frameOrdinal % _config.sampleEveryNFrames == 0);

    // Invariant counters are always reset, regardless of _recording.
    _staleQueuedEntryDiscarded.store(0, std::memory_order_release);
    _successorReadyTransitionSkipped.store(0, std::memory_order_release);
    _successorCancelTransitionSkipped.store(0, std::memory_order_release);
    _remainingDepsUnderflowAttempt.store(0, std::memory_order_release);
    _dispatchReadyToQueuedFailed.store(0, std::memory_order_release);
    _duplicateCompletionAttempt.store(0, std::memory_order_release);
    _scopeRemainingUnderflowAttempt.store(0, std::memory_order_release);
    _simulateRemainingUnderflowAttempt.store(0, std::memory_order_release);

    if (!_recording)
    {
        return;
    }

    EnsureSlotCapacity(graph.nodes.size());
    ResetSlots(graph.nodes.size());
    _activeNodes.store(0, std::memory_order_release);
    _maxActiveNodes.store(0, std::memory_order_release);
    _executedNodes.store(0, std::memory_order_release);
    _frameStartNs = NowNs();
}

void TaskExecutorDiagnosticsRecorder::EndFrame(
    const FrameTaskGraph& graph,
    const ExecutionSourceRegistry& sourceRegistry)
{
    // Invariant counters are always snapshotted, even when performance
    // recording is disabled. This allows tests to read them via GetLastFrame().
    _lastFrame.invariants.staleQueuedEntryDiscarded =
        _staleQueuedEntryDiscarded.load(std::memory_order_acquire);
    _lastFrame.invariants.successorReadyTransitionSkipped =
        _successorReadyTransitionSkipped.load(std::memory_order_acquire);
    _lastFrame.invariants.successorCancelTransitionSkipped =
        _successorCancelTransitionSkipped.load(std::memory_order_acquire);
    _lastFrame.invariants.remainingDepsUnderflowAttempt =
        _remainingDepsUnderflowAttempt.load(std::memory_order_acquire);
    _lastFrame.invariants.dispatchReadyToQueuedFailed =
        _dispatchReadyToQueuedFailed.load(std::memory_order_acquire);
    _lastFrame.invariants.duplicateCompletionAttempt =
        _duplicateCompletionAttempt.load(std::memory_order_acquire);
    _lastFrame.invariants.scopeRemainingUnderflowAttempt =
        _scopeRemainingUnderflowAttempt.load(std::memory_order_acquire);
    _lastFrame.invariants.simulateRemainingUnderflowAttempt =
        _simulateRemainingUnderflowAttempt.load(std::memory_order_acquire);

    if (!_recording)
    {
        return;
    }

    const uint64_t frameEndNs = NowNs();
    BuildFrameSnapshot(graph, sourceRegistry, frameEndNs);

    if (_config.logFrameSummary)
    {
        EmitFrameSummary();
    }

    if (_config.writeCsv)
    {
        WriteCsvFiles();
    }

    _recording = false;
}

void TaskExecutorDiagnosticsRecorder::BeginNode(
    const ExecNodeRecord& node,
    ExecNodeId nodeId) noexcept
{
    (void)node;

    if (!_recording || nodeId >= _activeSlotCount)
    {
        return;
    }

    const uint32_t active =
        _activeNodes.fetch_add(1, std::memory_order_acq_rel) + 1;
    UpdateAtomicMax(_maxActiveNodes, active);

    NodeTimingSlot& slot = _slots[nodeId];
    const uint64_t nowNs = NowNs();
    slot.startNs.store(
        static_cast<int64_t>(nowNs - _frameStartNs),
        std::memory_order_release);
    slot.threadHash.store(
        static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())),
        std::memory_order_release);
}

void TaskExecutorDiagnosticsRecorder::EndNode(
    const ExecNodeRecord& node,
    ExecNodeId nodeId,
    ExecCallResult result) noexcept
{
    (void)node;

    if (!_recording || nodeId >= _activeSlotCount)
    {
        return;
    }

    const uint64_t nowNs = NowNs();
    NodeTimingSlot& slot = _slots[nodeId];
    slot.endNs.store(
        static_cast<int64_t>(nowNs - _frameStartNs),
        std::memory_order_release);
    slot.result.store(
        static_cast<uint32_t>(result),
        std::memory_order_release);

    _executedNodes.fetch_add(1, std::memory_order_acq_rel);
    _activeNodes.fetch_sub(1, std::memory_order_acq_rel);
}

uint64_t TaskExecutorDiagnosticsRecorder::NowNs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

double TaskExecutorDiagnosticsRecorder::ToUs(uint64_t ns) noexcept
{
    return static_cast<double>(ns) /
        static_cast<double>(kNsPerUs);
}

double TaskExecutorDiagnosticsRecorder::ToUsSigned(int64_t ns) noexcept
{
    return static_cast<double>(ns) /
        static_cast<double>(kNsPerUs);
}

const char* TaskExecutorDiagnosticsRecorder::PhaseName(ExecPhase phase) noexcept
{
    switch (phase) {
    case ExecPhase::Simulate: return "Simulate";
    case ExecPhase::Commit: return "Commit";
    case ExecPhase::LifecycleFlush: return "LifecycleFlush";
    case ExecPhase::Reconcile: return "Reconcile";
    default: return "None";
    }
}

const char* TaskExecutorDiagnosticsRecorder::KindName(ExecNodeKind kind) noexcept
{
    switch (kind) {
    case ExecNodeKind::StaticSystem: return "StaticSystem";
    case ExecNodeKind::DynamicTask: return "DynamicTask";
    case ExecNodeKind::StructuralApply: return "StructuralApply";
    case ExecNodeKind::DeferredStateApply: return "DeferredStateApply";
    case ExecNodeKind::PostCommitFinalize: return "PostCommitFinalize";
    case ExecNodeKind::LifecycleFlush: return "LifecycleFlush";
    case ExecNodeKind::Reconcile: return "Reconcile";
    default: return "None";
    }
}

const char* TaskExecutorDiagnosticsRecorder::ResultName(ExecCallResult result) noexcept
{
    return result == ExecCallResult::Success ? "Success" : "Failed";
}

void TaskExecutorDiagnosticsRecorder::UpdateAtomicMax(
    std::atomic<uint32_t>& target,
    uint32_t value) noexcept
{
    uint32_t current = target.load(std::memory_order_relaxed);
    while (current < value &&
           !target.compare_exchange_weak(
               current,
               value,
               std::memory_order_relaxed,
               std::memory_order_relaxed))
    {
    }
}

void TaskExecutorDiagnosticsRecorder::EnsureSlotCapacity(size_t nodeCount)
{
    if (nodeCount <= _slotCapacity)
    {
        return;
    }

    _slots = std::make_unique<NodeTimingSlot[]>(nodeCount);
    _slotCapacity = nodeCount;
}

void TaskExecutorDiagnosticsRecorder::ResetSlots(size_t nodeCount) noexcept
{
    _activeSlotCount = std::min(nodeCount, _slotCapacity);
    for (size_t i = 0; i < _activeSlotCount; ++i)
    {
        NodeTimingSlot& slot = _slots[i];
        slot.startNs.store(-1, std::memory_order_release);
        slot.endNs.store(-1, std::memory_order_release);
        slot.threadHash.store(0, std::memory_order_release);
        slot.result.store(
            static_cast<uint32_t>(ExecCallResult::Failed),
            std::memory_order_release);
    }
}

void TaskExecutorDiagnosticsRecorder::BuildFrameSnapshot(
    const FrameTaskGraph& graph,
    const ExecutionSourceRegistry& sourceRegistry,
    uint64_t frameEndNs)
{
    _lastFrame.Clear();
    _lastFrame.valid = true;
    _lastFrame.frameOrdinal = _frameOrdinal;
    _lastFrame.workerCount = _workerCount;
    _lastFrame.nodeCount = static_cast<uint32_t>(graph.nodes.size());
    _lastFrame.simulateNodeCount = graph.simulateNodeCount;
    _lastFrame.scopeCount = graph.scopeCount;
    _lastFrame.edgeCount = static_cast<uint32_t>(graph.edges.size() / 2);
    _lastFrame.executedNodes = _executedNodes.load(std::memory_order_acquire);
    _lastFrame.maxConcurrency = _maxActiveNodes.load(std::memory_order_acquire);
    _lastFrame.wallNs = frameEndNs - _frameStartNs;
    _lastFrame.minNodeNs = std::numeric_limits<uint64_t>::max();

    std::unordered_set<uint64_t> threads;
    if (_config.collectNodeTimings)
    {
        _lastFrame.nodes.reserve(graph.nodes.size());
    }

    for (ExecNodeId nodeId = 0;
         nodeId < static_cast<ExecNodeId>(graph.nodes.size()) &&
         nodeId < _activeSlotCount;
         ++nodeId)
    {
        const NodeTimingSlot& slot = _slots[nodeId];
        const int64_t startNs = slot.startNs.load(std::memory_order_acquire);
        const int64_t endNs = slot.endNs.load(std::memory_order_acquire);

        if (startNs < 0 || endNs < startNs)
        {
            continue;
        }

        const uint64_t durationNs = static_cast<uint64_t>(endNs - startNs);
        const uint64_t threadHash = slot.threadHash.load(std::memory_order_acquire);
        const ExecNodeRecord& node = graph.nodes[nodeId];

        _lastFrame.sumNodeNs += durationNs;
        _lastFrame.minNodeNs = std::min(_lastFrame.minNodeNs, durationNs);
        _lastFrame.maxNodeNs = std::max(_lastFrame.maxNodeNs, durationNs);
        if (threadHash != 0)
        {
            threads.insert(threadHash);
        }

        if (_config.collectNodeTimings)
        {
            TaskExecutorNodeDiagnostic nodeDiag{};
            nodeDiag.nodeId = nodeId;
            nodeDiag.scopeId = node.scopeId;
            nodeDiag.sourceToken = node.sourceToken;
            nodeDiag.phase = node.phase;
            nodeDiag.kind = node.kind;
            nodeDiag.flags = node.flags;
            nodeDiag.result = static_cast<ExecCallResult>(
                slot.result.load(std::memory_order_acquire));
            nodeDiag.threadHash = threadHash;
            nodeDiag.startNs = static_cast<uint64_t>(startNs);
            nodeDiag.endNs = static_cast<uint64_t>(endNs);
            nodeDiag.durationNs = durationNs;

            if (const ExecutionSourceDesc* desc = sourceRegistry.TryGet(node.sourceToken))
            {
                nodeDiag.debugName = desc->debugName;
            }

            _lastFrame.nodes.push_back(std::move(nodeDiag));
        }
    }

    if (_lastFrame.minNodeNs == std::numeric_limits<uint64_t>::max())
    {
        _lastFrame.minNodeNs = 0;
    }

    _lastFrame.distinctThreads = static_cast<uint32_t>(threads.size());

    const uint32_t effectiveWorkers = std::max<uint32_t>(1, _workerCount);
    if (_lastFrame.executedNodes == 0)
    {
        _lastFrame.idealNs = 0;
    }
    else
    {
        const uint64_t throughputIdeal =
            (_lastFrame.sumNodeNs + effectiveWorkers - 1) / effectiveWorkers;
        _lastFrame.idealNs = std::max(throughputIdeal, _lastFrame.maxNodeNs);
    }

    _lastFrame.overheadVsIdealNs =
        static_cast<int64_t>(_lastFrame.wallNs) -
        static_cast<int64_t>(_lastFrame.idealNs);

    const double capacityNs =
        static_cast<double>(_lastFrame.wallNs) *
        static_cast<double>(effectiveWorkers);
    _lastFrame.workerEfficiency =
        capacityNs > 0.0
            ? static_cast<double>(_lastFrame.sumNodeNs) / capacityNs
            : 0.0;
}

void TaskExecutorDiagnosticsRecorder::EmitFrameSummary() const
{
    if (!_lastFrame.valid)
    {
        return;
    }

    FWLOG_INFO(
        kLogCategory,
        "frame=%llu workers=%u nodes=%u executed=%u wallUs=%.2f sumNodeUs=%.2f maxNodeUs=%.2f overheadIdealUs=%.2f efficiency=%.3f maxConcurrency=%u threads=%u",
        static_cast<unsigned long long>(_lastFrame.frameOrdinal),
        _lastFrame.workerCount,
        _lastFrame.nodeCount,
        _lastFrame.executedNodes,
        ToUs(_lastFrame.wallNs),
        ToUs(_lastFrame.sumNodeNs),
        ToUs(_lastFrame.maxNodeNs),
        ToUsSigned(_lastFrame.overheadVsIdealNs),
        _lastFrame.workerEfficiency,
        _lastFrame.maxConcurrency,
        _lastFrame.distinctThreads);
}

void TaskExecutorDiagnosticsRecorder::WriteCsvFiles()
{
    WriteSummaryCsv();

    if (_config.collectNodeTimings)
    {
        WriteNodeCsv();
    }
}

void TaskExecutorDiagnosticsRecorder::WriteSummaryCsv()
{
    if (_config.summaryCsvPath.empty())
    {
        return;
    }

    std::filesystem::path path{ _config.summaryCsvPath };
    if (path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    const bool needsHeader =
        !_summaryHeaderWritten &&
        (!std::filesystem::exists(path) || std::filesystem::file_size(path) == 0);

    std::ofstream out(path, std::ios::out | std::ios::app);
    if (!out.is_open())
    {
        FWLOG_WARN(kLogCategory, "failed to open summary csv (%s)", _config.summaryCsvPath.c_str());
        return;
    }

    out << std::fixed << std::setprecision(3);
    if (needsHeader)
    {
        out
            << "frame,workers,node_count,simulate_node_count,scope_count,edge_count,"
            << "wall_us,sum_node_us,min_node_us,max_node_us,ideal_us,"
            << "overhead_vs_ideal_us,worker_efficiency,max_concurrency,"
            << "distinct_threads,executed_nodes\n";
    }

    out
        << _lastFrame.frameOrdinal << ','
        << _lastFrame.workerCount << ','
        << _lastFrame.nodeCount << ','
        << _lastFrame.simulateNodeCount << ','
        << _lastFrame.scopeCount << ','
        << _lastFrame.edgeCount << ','
        << ToUs(_lastFrame.wallNs) << ','
        << ToUs(_lastFrame.sumNodeNs) << ','
        << ToUs(_lastFrame.minNodeNs) << ','
        << ToUs(_lastFrame.maxNodeNs) << ','
        << ToUs(_lastFrame.idealNs) << ','
        << ToUsSigned(_lastFrame.overheadVsIdealNs) << ','
        << _lastFrame.workerEfficiency << ','
        << _lastFrame.maxConcurrency << ','
        << _lastFrame.distinctThreads << ','
        << _lastFrame.executedNodes << '\n';

    _summaryHeaderWritten = true;
}

void TaskExecutorDiagnosticsRecorder::WriteNodeCsv()
{
    if (_config.nodeCsvPath.empty())
    {
        return;
    }

    std::filesystem::path path{ _config.nodeCsvPath };
    if (path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    const bool needsHeader =
        !_nodeHeaderWritten &&
        (!std::filesystem::exists(path) || std::filesystem::file_size(path) == 0);

    std::ofstream out(path, std::ios::out | std::ios::app);
    if (!out.is_open())
    {
        FWLOG_WARN(kLogCategory, "failed to open node csv (%s)", _config.nodeCsvPath.c_str());
        return;
    }

    out << std::fixed << std::setprecision(3);
    if (needsHeader)
    {
        out
            << "frame,node_id,scope_id,source_token,phase,kind,flags,result,"
            << "thread_hash,start_us,end_us,duration_us,debug_name\n";
    }

    for (const TaskExecutorNodeDiagnostic& node : _lastFrame.nodes)
    {
        out
            << _lastFrame.frameOrdinal << ','
            << node.nodeId << ','
            << node.scopeId << ','
            << node.sourceToken << ','
            << PhaseName(node.phase) << ','
            << KindName(node.kind) << ','
            << node.flags << ','
            << ResultName(node.result) << ','
            << node.threadHash << ','
            << ToUs(node.startNs) << ','
            << ToUs(node.endNs) << ','
            << ToUs(node.durationNs) << ','
            << '"';

        for (char ch : node.debugName)
        {
            if (ch == '"')
            {
                out << "\"\"";
            }
            else
            {
                out << ch;
            }
        }

        out << '"' << '\n';
    }

    _nodeHeaderWritten = true;
}
