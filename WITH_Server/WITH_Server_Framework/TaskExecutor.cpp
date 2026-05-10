#include "pch.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>
#include <utility>

#include "DynamicTaskScheduler.h"
#include "FrameworkLog.h"

namespace
{
    constexpr const char* kLogCategory = "Executor";
    constexpr std::chrono::microseconds kNetworkPrePollTimeout{ 0 };
    constexpr std::chrono::microseconds kNetworkIdleWaitTimeout{ 1000 };

    // TEMP_TASKEXECUTOR_DEBUG: readable state names for temporary executor race diagnostics.
    const char* DebugNodeStateName(ExecNodeState state) noexcept
    {
        switch (state)
        {
        case ExecNodeState::NotReady: return "NotReady";
        case ExecNodeState::Ready: return "Ready";
        case ExecNodeState::Queued: return "Queued";
        case ExecNodeState::Running: return "Running";
        case ExecNodeState::Succeeded: return "Succeeded";
        case ExecNodeState::Failed: return "Failed";
        case ExecNodeState::Canceled: return "Canceled";
        case ExecNodeState::Skipped: return "Skipped";
        default: return "Unknown";
        }
    }

    // TEMP_TASKEXECUTOR_DEBUG: readable scope phase names for temporary executor race diagnostics.
    const char* DebugScopePhaseName(ExecScopePhase phase) noexcept
    {
        switch (phase)
        {
        case ExecScopePhase::Open: return "Open";
        case ExecScopePhase::CancelRequested: return "CancelRequested";
        case ExecScopePhase::Draining: return "Draining";
        case ExecScopePhase::Closed: return "Closed";
        default: return "Unknown";
        }
    }
}

TaskExecutor::ExecutingNodeGuard::ExecutingNodeGuard(TaskExecutor& inExecutor) noexcept
    : executor(inExecutor)
{
    executor._activeExecutingNodes.fetch_add(1, std::memory_order_acq_rel);
}

TaskExecutor::ExecutingNodeGuard::~ExecutingNodeGuard()
{
    executor._activeExecutingNodes.fetch_sub(1, std::memory_order_acq_rel);
    executor.TryPublishSimulateDone();
    executor.NotifyProgress();
}

TaskExecutor::TaskExecutor(uint32_t workerCount)
{
    if (workerCount > 0)
        (void)Initialize(workerCount);
}

TaskExecutor::~TaskExecutor()
{
    Shutdown();
}

bool TaskExecutor::Initialize(uint32_t workerCount)
{
    bool expected{ false };
    if (!_initialized.compare_exchange_strong(expected, true))
    {
        FWLOG_WARN(kLogCategory, "Initialize called but already initialized");
        return true;
    }

    try
    {
        if (!_pool.Start(workerCount, &TaskExecutor::WorkerPumpEntry, this))
        {
            FWLOG_ERROR(kLogCategory, "ThreadPool::Start failed (workerCount=%u)", workerCount);
            _initialized.store(false);
            return false;
        }
    }
    catch (...)
    {
        FWLOG_FATAL(kLogCategory, "ThreadPool::Start threw exception (workerCount=%u)", workerCount);
        _initialized.store(false);
        throw;
    }

    // 풀이 확정한 실제 워커 수에 맞춰 per-worker deque를 할당한다.
    // WorkerPump가 _workerDeques에 접근하는 시점은 _frameBound가 true가 된 이후이므로,
    // Start 반환 후 할당해도 race 없이 안전하다.
    const uint32_t actualWorkerCount = _pool.WorkerCount();
    _workerDeques.clear();
    _workerDeques.reserve(actualWorkerCount);
    for (uint32_t i = 0; i < actualWorkerCount; ++i)
        _workerDeques.push_back(
            std::make_unique<LFWSDeque<ExecNodeId, kWorkerDequeCapacity>>());

    FWLOG_INFO(kLogCategory, "Initialized with %u workers", actualWorkerCount);
    return true;
}

void TaskExecutor::Shutdown() noexcept
{
    bool expected{ true };
    if (!_initialized.compare_exchange_strong(expected, false))
    {
        return;
    }

    FWLOG_INFO(kLogCategory, "Shutdown begin");

    _frameBound.store(false);
    _pool.Stop();

    UnbindFrame();

    FWLOG_INFO(kLogCategory, "Shutdown complete");
}

bool TaskExecutor::ExecuteFrame(
    FrameExecContext& frameCtx,
    ExecRuntimeState& runtime,
    const ExecutionSourceRegistry& sourceRegistry)
{
    // TEMP_TASKEXECUTOR_DEBUG: correlate all queue/state logs for one ExecuteFrame call.
    const uint64_t debugFrameId =
        _debugNextFrameId.fetch_add(1, std::memory_order_relaxed);
    _debugCurrentFrameId.store(debugFrameId, std::memory_order_release);

    if (!IsInitialized())
    {
        FWLOG_ERROR(kLogCategory, "ExecuteFrame called but executor is not initialized");
        return false;
    }

    if (!ValidateFrameInputs(frameCtx, runtime))
    {
        FWLOG_ERROR(kLogCategory, "ExecuteFrame - ValidateFrameInputs failed (nodes=%zu, scopes=%zu)",
            runtime.nodes.size(), runtime.scopes.size());
        return false;
    }

    FWLOG_DEBUG(kLogCategory, "ExecuteFrame begin (nodes=%zu, simulateNodes=%u, scopes=%u)",
        frameCtx.graph->nodes.size(),
        frameCtx.graph->simulateNodeCount,
        frameCtx.graph->scopeCount);

    // TEMP_TASKEXECUTOR_DEBUG: frame lifecycle breadcrumb.
    FWLOG_INFO(kLogCategory,
        "TEMP_TASKEXECUTOR_DEBUG ExecuteFrame begin (frameId=%llu, nodes=%zu, simulateNodes=%u, scopes=%u, activePumps=%u, activeNodes=%u)",
        static_cast<unsigned long long>(debugFrameId),
        frameCtx.graph->nodes.size(),
        frameCtx.graph->simulateNodeCount,
        frameCtx.graph->scopeCount,
        _debugActiveWorkerPumps.load(std::memory_order_acquire),
        _activeExecutingNodes.load(std::memory_order_acquire));

    BindFrame(frameCtx, runtime, sourceRegistry);
    InitializeRuntimeForFrame();
    _diagnostics.BeginFrame(
        *frameCtx.graph,
        _pool.WorkerCount(),
        _diagnosticsFrameOrdinal++);

    SeedInitialSimulateNodes();
    if (!runtime.signals.simulatePhaseDone.load(std::memory_order_acquire))
    {
        _frameBound.store(true, std::memory_order_release);
        FWLOG_INFO(kLogCategory,
            "TEMP_TASKEXECUTOR_DEBUG FrameExecutionEnabled (frameId=%llu, activePumps=%u, activeNodes=%u, frameBound=1)",
            static_cast<unsigned long long>(debugFrameId),
            _debugActiveWorkerPumps.load(std::memory_order_acquire),
            _activeExecutingNodes.load(std::memory_order_acquire));
        NotifyAllWorkers();
    }
    WaitForSimulateDone();

    FWLOG_TRACE(kLogCategory, "Simulate phase done, entering serial phases");

    const FrameTaskGraph& graph = Graph();

    RunSerialPhase(graph.commitPlan, ExecPhase::Commit);
    RunScopeSerialPhase(graph.commitScopePlan, ExecPhase::Commit);
    FinalizeScopeClosures();

    RunSerialPhase(graph.lifecycleFlushPlan, ExecPhase::LifecycleFlush);
    RunScopeSerialPhase(graph.lifecycleFlushScopePlan, ExecPhase::LifecycleFlush);
    FinalizeScopeClosures();

    RunSerialPhase(graph.reconcilePlan, ExecPhase::Reconcile);
    RunScopeSerialPhase(graph.reconcileScopePlan, ExecPhase::Reconcile);
    FinalizeScopeClosures();

    _diagnostics.EndFrame(graph, sourceRegistry);

    UnbindFrame();

    FWLOG_DEBUG(kLogCategory, "ExecuteFrame complete");
    // TEMP_TASKEXECUTOR_DEBUG: frame lifecycle breadcrumb.
    FWLOG_INFO(kLogCategory,
        "TEMP_TASKEXECUTOR_DEBUG ExecuteFrame complete (frameId=%llu, activePumps=%u, activeNodes=%u)",
        static_cast<unsigned long long>(debugFrameId),
        _debugActiveWorkerPumps.load(std::memory_order_acquire),
        _activeExecutingNodes.load(std::memory_order_acquire));
    return true;
}

void TaskExecutor::SetDiagnosticsConfig(TaskExecutorDiagnosticsConfig config)
{
    _diagnostics.Configure(std::move(config));
}

const TaskExecutorFrameDiagnostics& TaskExecutor::GetLastFrameDiagnostics() const noexcept
{
    return _diagnostics.GetLastFrame();
}

bool TaskExecutor::WorkerPumpEntry(void* ctx, uint32_t workerIdx)
{
    if (ctx == nullptr)
        return false;

    return static_cast<TaskExecutor*>(ctx)->WorkerPump(workerIdx);
}

bool TaskExecutor::WorkerPump(uint32_t workerIdx)
{
    // TEMP_TASKEXECUTOR_DEBUG: detect workers still inside a frame while queues/binding are reset.
    struct DebugPumpGuard
    {
        std::atomic<uint32_t>& counter;
        ~DebugPumpGuard()
        {
            counter.fetch_sub(1, std::memory_order_acq_rel);
        }
    };

    _debugActiveWorkerPumps.fetch_add(1, std::memory_order_acq_rel);
    DebugPumpGuard debugPumpGuard{ _debugActiveWorkerPumps };

    const bool hasNetworkBackend = _networkBackend != nullptr;
    bool handledNetworkIo = false;
    if (hasNetworkBackend)
    {
        // Drain already-ready IOCP completions before static ECS work without
        // blocking the worker. This keeps inbound DynamicTask injection from
        // starving behind a long simulate queue.
        handledNetworkIo =
            _networkBackend->WaitForWork(workerIdx, kNetworkPrePollTimeout);
    }

    if (!_frameBound.load(std::memory_order_acquire))
    {
        if (handledNetworkIo)
            return true;

        if (hasNetworkBackend && !handledNetworkIo)
        {
            // Short between-frame pump. ExecuteFrame wakes workers through the
            // thread-pool CV, but a worker blocked in IOCP is only released by
            // completion, WakeWorker(), or this timeout.
            //
            // TODO: Wake the IOCP side explicitly when a new frame is published,
            // or reserve dedicated IO pump workers, so frame-start latency is not
            // bounded by kNetworkIdleWaitTimeout.
            (void)_networkBackend->WaitForWork(workerIdx, kNetworkIdleWaitTimeout);
        }
        return false;
    }

    // Phase 1: 자신의 deque에서 TryPop (lock 없음, CAS 없음 — 가장 빠른 경로)
    {
        std::optional<ExecNodeId> opt = _workerDeques[workerIdx]->TryPop();
        if (opt.has_value())
        {
            // TEMP_TASKEXECUTOR_DEBUG: queue-consume breadcrumb.
            if (const ExecNodeRuntime* nodeRt = Runtime().TryGetNode(*opt))
            {
                FWLOG_INFO(kLogCategory,
                    "TEMP_TASKEXECUTOR_DEBUG QueueConsume (frameId=%llu, source=ownerDeque, worker=%u, nodeId=%u, nodeState=%s, frameBound=%u)",
                    static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                    workerIdx,
                    *opt,
                    DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)),
                    _frameBound.load(std::memory_order_acquire) ? 1u : 0u);
            }
            ExecuteNode(*opt, workerIdx);
            return true;
        }
    }

    // Phase 2: 다른 워커 deque에서 TrySteal (CAS만 사용, lock 없음)
    {
        const uint32_t workerCount = static_cast<uint32_t>(_workerDeques.size());
        for (uint32_t v = 1; v < workerCount; ++v)
        {
            const uint32_t victimIdx = (workerIdx + v) % workerCount;
            std::optional<ExecNodeId> opt = _workerDeques[victimIdx]->TrySteal();
            if (opt.has_value())
            {
                // TEMP_TASKEXECUTOR_DEBUG: queue-consume breadcrumb.
                if (const ExecNodeRuntime* nodeRt = Runtime().TryGetNode(*opt))
                {
                    FWLOG_INFO(kLogCategory,
                        "TEMP_TASKEXECUTOR_DEBUG QueueConsume (frameId=%llu, source=steal, worker=%u, victim=%u, nodeId=%u, nodeState=%s, frameBound=%u)",
                        static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                        workerIdx,
                        victimIdx,
                        *opt,
                        DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)),
                        _frameBound.load(std::memory_order_acquire) ? 1u : 0u);
                }
                ExecuteNode(*opt, workerIdx);
                return true;
            }
        }
    }

    // Phase 3: lock-free injection 큐에서 try_pop (초기 시드 / deque overflow fallback)
    {
        ExecNodeId nodeId = InvalidExecNodeId;
        if (_injectQueue.try_pop(nodeId))
        {
            // TEMP_TASKEXECUTOR_DEBUG: queue-consume breadcrumb.
            if (const ExecNodeRuntime* nodeRt = Runtime().TryGetNode(nodeId))
            {
                FWLOG_INFO(kLogCategory,
                    "TEMP_TASKEXECUTOR_DEBUG QueueConsume (frameId=%llu, source=injectQueue, worker=%u, nodeId=%u, nodeState=%s, frameBound=%u)",
                    static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                    workerIdx,
                    nodeId,
                    DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)),
                    _frameBound.load(std::memory_order_acquire) ? 1u : 0u);
            }
            ExecuteNode(nodeId, workerIdx);
            return true;
        }
    }

    // Phase 4: CompletionQueue 전체 드레인 — Suspended 노드 재개
    ProcessCompletions(workerIdx);

    // Simulate deadline 초과 시 단일 워커가 Sweep 트리거
    if (_frameState.IsDeadlineExceeded())
    {
        bool expected = false;
        if (_frameState.sweepTriggered.compare_exchange_strong(
            expected, true,
            std::memory_order_acq_rel))
        {
            TriggerSweep();
        }
    }

    if (hasNetworkBackend)
    {
        const bool handledIdleNetworkIo =
            _networkBackend->WaitForWork(workerIdx, kNetworkIdleWaitTimeout);
        return handledNetworkIo || handledIdleNetworkIo;
    }

    return handledNetworkIo;
}

bool TaskExecutor::ValidateFrameInputs(
    const FrameExecContext& frameCtx,
    const ExecRuntimeState& runtime) const noexcept
{
    if (!frameCtx.IsValid())
        return false;

    if (frameCtx.graph == nullptr)
        return false;

    const FrameTaskGraph& graph = *frameCtx.graph;

    if (runtime.nodes.size() < graph.nodes.size())
        return false;

    if (runtime.scopes.size() < graph.scopeCount)
        return false;

    return true;
}

void TaskExecutor::BindFrame(
    FrameExecContext& frameCtx,
    ExecRuntimeState& runtime,
    const ExecutionSourceRegistry& sourceRegistry) noexcept
{
    _frameBound.store(false, std::memory_order_release);

    _binding.frame = &frameCtx;
    _binding.runtime = &runtime;
    _binding.sources = &sourceRegistry;

    // 워커가 아직 깨어있지 않은 상태(BindFrame 호출 시점은 항상 직전 프레임 완료 후)이므로
    // 경쟁 없이 Reset을 호출할 수 있다.
    for (auto& deque : _workerDeques)
        deque->Reset();

    _injectQueue.clear();

    {
        std::lock_guard lock{ _mainQueueMtx };
        _mainThreadReadyQueue.clear();
    }

    // TEMP_TASKEXECUTOR_DEBUG: frame binding breadcrumb.
    FWLOG_INFO(kLogCategory,
        "TEMP_TASKEXECUTOR_DEBUG BindFrame prepared (frameId=%llu, nodes=%zu, scopes=%zu, workerDeques=%zu, activePumps=%u, activeNodes=%u)",
        static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
        frameCtx.graph != nullptr ? frameCtx.graph->nodes.size() : 0,
        runtime.scopes.size(),
        _workerDeques.size(),
        _debugActiveWorkerPumps.load(std::memory_order_acquire),
        _activeExecutingNodes.load(std::memory_order_acquire));
}

void TaskExecutor::UnbindFrame() noexcept
{
    const uint32_t activeNodesAtBegin =
        _activeExecutingNodes.load(std::memory_order_acquire);

    // TEMP_TASKEXECUTOR_DEBUG: frame unbinding breadcrumb before queue/binding reset.
    FWLOG_INFO(kLogCategory,
        "TEMP_TASKEXECUTOR_DEBUG UnbindFrame begin (frameId=%llu, activePumps=%u, activeNodes=%u, frameBound=%u)",
        static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
        _debugActiveWorkerPumps.load(std::memory_order_acquire),
        activeNodesAtBegin,
        _frameBound.load(std::memory_order_acquire) ? 1u : 0u);

    if (activeNodesAtBegin != 0)
    {
        FWLOG_ERROR(kLogCategory,
            "TaskExecutor frame quiescence invariant violated before unbind "
            "(frameId=%llu, activeNodes=%u)",
            static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
            activeNodesAtBegin);
    }

    _frameBound.store(false, std::memory_order_release);

    while (_debugActiveWorkerPumps.load(std::memory_order_acquire) != 0 ||
        _activeExecutingNodes.load(std::memory_order_acquire) != 0)
    {
        std::this_thread::yield();
    }

    // _pool.Stop() 또는 프레임 완료 후 호출되므로 워커는 이미 idle 또는 종료 상태이다.
    for (auto& deque : _workerDeques)
        deque->Reset();

    _injectQueue.clear();

    {
        std::lock_guard lock{ _mainQueueMtx };
        _mainThreadReadyQueue.clear();
    }

    _binding = {};

    // TEMP_TASKEXECUTOR_DEBUG: frame unbinding breadcrumb after queue/binding reset.
    FWLOG_INFO(kLogCategory,
        "TEMP_TASKEXECUTOR_DEBUG UnbindFrame end (frameId=%llu, activePumps=%u, activeNodes=%u)",
        static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
        _debugActiveWorkerPumps.load(std::memory_order_acquire),
        _activeExecutingNodes.load(std::memory_order_acquire));
    _debugCurrentFrameId.store(0, std::memory_order_release);
}

void TaskExecutor::InitializeRuntimeForFrame()
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    std::vector<uint32_t> nodeDepCounts(graph.nodes.size(), 0);
    std::vector<uint32_t> scopeNodeCounts(graph.scopeCount, 0);

    for (ExecNodeId nodeId = 0;
        nodeId < static_cast<ExecNodeId>(graph.nodes.size());
        ++nodeId)
    {
        const ExecNodeRecord& node = graph.nodes[nodeId];

        if (node.phase == ExecPhase::Simulate)
            nodeDepCounts[nodeId] = node.predCount;
        else
            nodeDepCounts[nodeId] = 0;

        if (graph.IsValidScopeId(node.scopeId))
            ++scopeNodeCounts[node.scopeId];
    }

    runtime.ResetAll(nodeDepCounts, scopeNodeCounts, graph.simulateNodeCount);

    using Clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds kSimulateFrameBudget{ 16 };
    _frameState.Reset(Clock::now() + kSimulateFrameBudget * 70 / 100);
}

void TaskExecutor::SeedInitialSimulateNodes()
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    for (ExecNodeId nodeId = 0;
        nodeId < static_cast<ExecNodeId>(graph.nodes.size());
        ++nodeId)
    {
        const ExecNodeRecord& node = graph.nodes[nodeId];
        if (node.phase != ExecPhase::Simulate)
            continue;

        ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
        ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);
        if (nodeRt == nullptr || scopeRt == nullptr)
        {
            FWLOG_ERROR(kLogCategory,
                "SeedInitialSimulateNodes - nullptr runtime (nodeId=%u, scopeId=%u)",
                nodeId, node.scopeId);
            continue;
        }

        if (nodeRt->remainingDeps.load() != 0)
            continue;

        const ExecScopePhase scopePhase = scopeRt->phase.load();

        if (IsExecutableScopePhase(scopePhase))
        {
            if (TryTransitionNode(*nodeRt, ExecNodeState::NotReady, ExecNodeState::Ready))
            {
                // 초기 시드는 메인 스레드에서 수행하므로 kNoWorkerIdx를 전달한다.
                // EnqueueReadyNode가 공유 큐에 삽입하고, 워커들이 steal해 간다.
                DispatchNode(nodeId, kNoWorkerIdx);
            }
        }
        else
        {
            const ExecNodeState terminal = SelectCancelTerminalState(node);
            if (TryTransitionNode(*nodeRt, ExecNodeState::NotReady, terminal))
            {
                CompleteNodeTerminal(nodeId, terminal);
            }
        }
    }

    if (graph.simulateNodeCount == 0)
    {
        TryPublishSimulateDone();
    }
}

void TaskExecutor::RunSerialPhase(
    const ExecRange& range,
    ExecPhase expectedPhase)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    if (range.IsEmpty())
        return;

    assert(graph.IsValidSerialRange(range));

    for (uint32_t i = 0; i < range.count; ++i)
    {
        const ExecNodeId nodeId =
            graph.serialExecutionOrder[range.begin + i];
        assert(graph.IsValidNodeId(nodeId));

        const ExecNodeRecord& node = graph.nodes[nodeId];
        assert(node.phase == expectedPhase);

        ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
        ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);
        assert(nodeRt != nullptr);
        assert(scopeRt != nullptr);

        const ExecScopePhase scopePhase = scopeRt->phase.load();

        if (!IsExecutableScopePhase(scopePhase))
        {
            const ExecNodeState terminal = SelectCancelTerminalState(node);
            const bool ok =
                TryTransitionNode(*nodeRt, ExecNodeState::NotReady, terminal);
            assert(ok);

            CompleteNodeTerminal(nodeId, terminal);
            continue;
        }

        const bool entered =
            TryTransitionNode(*nodeRt, ExecNodeState::NotReady, ExecNodeState::Running);
        assert(entered);

        _diagnostics.BeginNode(node, nodeId);
        const ExecCallResult callResult = InvokeNode(node, nodeId);
        _diagnostics.EndNode(node, nodeId, callResult);

        if (callResult == ExecCallResult::Success)
        {
            const bool ok = 
                TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Succeeded);
            assert(ok);
            CompleteNodeTerminal(nodeId, ExecNodeState::Succeeded);
        }
        else
        {
            const bool ok = 
                TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Failed);
            assert(ok);

            MarkScopeFailedAndCancelRequested(node.scopeId);
            CompleteNodeTerminal(nodeId, ExecNodeState::Failed);
        }
    }
}

void TaskExecutor::RunScopeSerialPhase(
    const ExecRange& range,
    ExecPhase expectedPhase)
{
    const FrameTaskGraph& graph = Graph();
    FrameExecContext& frame = Frame();

    if (range.IsEmpty())
        return;

    assert(graph.IsValidSerialScopeRange(range));
    assert(frame.ops != nullptr);

    for (uint32_t i = 0; i < range.count; ++i)
    {
        const ExecScopeId scopeId =
            graph.serialScopeOrder[range.begin + i];

        assert(frame.IsValidScopeId(scopeId));

        switch (expectedPhase) {
        case ExecPhase::Commit:
            frame.ops->CommitScope(scopeId, frame.runtimeByScope);
            break;

        case ExecPhase::LifecycleFlush:
            frame.ops->FlushLifecycle(scopeId, frame.runtimeByScope);
            break;

        case ExecPhase::Reconcile:
            frame.ops->ReconcileScope(scopeId, frame.runtimeByScope);
            break;

        default:
            assert(false);
            break;
        }
    }
}

void TaskExecutor::FinalizeScopeClosures() noexcept
{
    ExecRuntimeState& runtime = Runtime();

    for (ExecScopeId scopeId = 0;
        scopeId < static_cast<ExecScopeId>(runtime.scopes.size());
        ++scopeId)
    {
        ExecScopeRuntime* scopeRt = runtime.TryGetScope(scopeId);
        if (scopeRt == nullptr)
            continue;

        if (!scopeRt->closeCandidate.load())
            continue;

        if (scopeRt->remainingNodes.load() != 0)
            continue;

        ExecScopePhase current = scopeRt->phase.load();

        while (current != ExecScopePhase::Closed)
        {
            if (scopeRt->phase.compare_exchange_strong(
                current, ExecScopePhase::Closed))
            {
                break;
            }
        }
    }
}

void TaskExecutor::DrainMainThreadQueue()
{
    while (true)
    {
        ExecNodeId nodeId = InvalidExecNodeId;

        {
            std::lock_guard lock{ _mainQueueMtx };
            if (_mainThreadReadyQueue.empty())
                break;

            nodeId = _mainThreadReadyQueue.front();
            _mainThreadReadyQueue.pop_front();
        }

        // 메인 스레드 컨텍스트이므로 kNoWorkerIdx 전달.
        // 후계자는 공유 큐에 삽입되어 워커들이 steal한다.
        ExecuteNode(nodeId, kNoWorkerIdx);
    }
}

void TaskExecutor::WaitForSimulateDone()
{
    ExecRuntimeState& runtime = Runtime();

    while (!runtime.signals.simulatePhaseDone.load(std::memory_order_acquire))
    {
        // 메인 스레드 전용 노드를 먼저 소진한다.
        DrainMainThreadQueue();

        if (runtime.signals.simulatePhaseDone.load(std::memory_order_acquire))
            break;

        // 워커 진행 또는 500µs 타임아웃 후 재확인한다.
        // 타임아웃은 MainThreadOnly 노드가 삽입됐을 때 즉시 반응하기 위함이다.
        std::unique_lock lock{ _progressMtx };
        _progressCv.wait_for(
            lock, std::chrono::microseconds(500),
            [&]() { return runtime.signals.simulatePhaseDone.load(); });

        // Suspended 노드가 있을 때 워커가 CompletionQueue 및 deadline을
        // 주기적으로 확인할 수 있도록 깨운다 (워커가 idle cv.wait 중일 때).
        NotifyWork();
    }

    // simulatePhaseDone 직전에 삽입된 잔여 메인 스레드 노드를 정리한다.
    while (_debugActiveWorkerPumps.load(std::memory_order_acquire) != 0 ||
        _activeExecutingNodes.load(std::memory_order_acquire) != 0)
    {
        DrainMainThreadQueue();
        std::this_thread::yield();
    }

    DrainMainThreadQueue();
}

void TaskExecutor::EnqueueReadyNode(ExecNodeId nodeId, uint32_t workerIdx)
{
    // 유효한 워커 인덱스이면 해당 워커의 deque에 직접 Push하여
    // 캐시 지역성을 유지한다 (lock 없음, CAS 없음).
    if (workerIdx != kNoWorkerIdx)
    {
        if (_workerDeques[workerIdx]->TryPush(nodeId))
        {
            // TEMP_TASKEXECUTOR_DEBUG: queue-insert breadcrumb.
            FWLOG_INFO(kLogCategory,
                "TEMP_TASKEXECUTOR_DEBUG QueueInsert (frameId=%llu, target=workerDeque, worker=%u, nodeId=%u)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                workerIdx,
                nodeId);
            NotifyWork();
            return;
        }

        // deque 가득 참 → _injectQueue로 fallback (PPL concurrent_queue, lock-free unbounded)
        FWLOG_WARN(kLogCategory,
            "EnqueueReadyNode - worker deque full, falling back to injectQueue (workerIdx=%u, nodeId=%u)",
            workerIdx, nodeId);
    }

    // 메인 스레드 컨텍스트(kNoWorkerIdx) 또는 deque overflow 경로.
    // concurrent_queue::push는 항상 성공한다 (unbounded).
    _injectQueue.push(nodeId);

    // TEMP_TASKEXECUTOR_DEBUG: queue-insert breadcrumb.
    FWLOG_INFO(kLogCategory,
        "TEMP_TASKEXECUTOR_DEBUG QueueInsert (frameId=%llu, target=injectQueue, worker=%u, nodeId=%u)",
        static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
        workerIdx,
        nodeId);

    NotifyWork();
}

void TaskExecutor::DispatchNode(ExecNodeId nodeId, uint32_t workerIdx)
{
    ExecRuntimeState& runtime = Runtime();
    ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
    if (nodeRt == nullptr)
    {
        FWLOG_ERROR(kLogCategory,
            "DispatchNode - TryGetNode returned null (nodeId=%u)", nodeId);
        return;
    }

    const bool ok =
        TryTransitionNode(*nodeRt, ExecNodeState::Ready, ExecNodeState::Queued);

    if (!ok)
    {
        // SPEC-EXEC-QUEUE-001 §개선방향 §1: dispatchReadyToQueuedFailed 카운터 증가.
        _diagnostics.RecordDispatchReadyToQueuedFailed();
        FWLOG_WARN(kLogCategory,
            "DispatchNode Ready->Queued failed "
            "(frameId=%llu, nodeId=%u, worker=%u, currentState=%s, remainingDeps=%u)",
            static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
            nodeId,
            workerIdx,
            DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)),
            nodeRt->remainingDeps.load(std::memory_order_acquire));
        return;
    }

    // MainThreadOnly 노드는 전용 큐에 삽입하고 메인 스레드를 깨운다.
    // 워커 스레드가 이 큐에 접근하는 경로는 없다.
    const FrameTaskGraph& graph = Graph();
    assert(graph.IsValidNodeId(nodeId));
    const ExecNodeRecord& node = graph.nodes[nodeId];

    if (HasAnyNodeFlag(node.flags, ExecNodeFlag_MainThreadOnly))
    {
        {
            std::lock_guard lock{ _mainQueueMtx };
            _mainThreadReadyQueue.push_back(nodeId);
        }
        // TEMP_TASKEXECUTOR_DEBUG: queue-insert breadcrumb.
        FWLOG_INFO(kLogCategory,
            "TEMP_TASKEXECUTOR_DEBUG QueueInsert (frameId=%llu, target=mainThreadQueue, worker=%u, nodeId=%u, state=%s)",
            static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
            workerIdx,
            nodeId,
            DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
        NotifyProgress();
    }
    else
    {
        EnqueueReadyNode(nodeId, workerIdx);
    }
}

void TaskExecutor::ExecuteNode(ExecNodeId nodeId, uint32_t workerIdx)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    assert(graph.IsValidNodeId(nodeId));

    const ExecNodeRecord& node = graph.nodes[nodeId];
    ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);

    assert(nodeRt != nullptr);
    assert(scopeRt != nullptr);

    ExecutingNodeGuard executingNodeGuard{ *this };

    FWLOG_TRACE(kLogCategory, "ExecuteNode begin (nodeId=%u, scopeId=%u, token=%u)",
        nodeId, node.scopeId, node.sourceToken);

    const ExecScopePhase scopePhase = scopeRt->phase.load();

    if (!IsExecutableScopePhase(scopePhase))
    {
        const ExecNodeState terminal = SelectCancelTerminalState(node);

        FWLOG_DEBUG(kLogCategory, "ExecuteNode canceled (nodeId=%u, scopePhase=%u, terminal=%u)",
            nodeId, static_cast<uint32_t>(scopePhase), static_cast<uint32_t>(terminal));

        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Queued, terminal);
        if (!ok)
        {
            // SPEC-EXEC-QUEUE-001 §필수불변식 4:
            // Queued→Cancel CAS 실패 = stale/duplicate queue entry. discard하고 반환.
            // remainingNodes / remainingSimulateNodes 를 감소시키면 안 된다.
            _diagnostics.RecordStaleQueuedEntry();
            FWLOG_DEBUG(kLogCategory,
                "ExecuteNode stale entry discarded (cancel path) "
                "(frameId=%llu, nodeId=%u, worker=%u, token=%u, currentState=%s)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                nodeId,
                workerIdx,
                node.sourceToken,
                DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
            return;
        }

        CompleteNodeTerminal(nodeId, terminal);
        return;
    }

    const bool canRun =
        TryTransitionNode(*nodeRt, ExecNodeState::Queued, ExecNodeState::Running);
    if (!canRun)
    {
        // SPEC-EXEC-QUEUE-001 §필수불변식 3, 4:
        // Queued→Running CAS 실패 = stale/duplicate queue entry. discard하고 반환.
        // 이는 executor 내부 assert 조건이 아니며, remainingNodes 를 감소시키면 안 된다.
        _diagnostics.RecordStaleQueuedEntry();
        FWLOG_DEBUG(kLogCategory,
            "ExecuteNode stale entry discarded (run path) "
            "(frameId=%llu, nodeId=%u, worker=%u, token=%u, currentState=%s)",
            static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
            nodeId,
            workerIdx,
            node.sourceToken,
            DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
        return;
    }

    _diagnostics.BeginNode(node, nodeId);
    const ExecCallResult callResult = InvokeNode(node, nodeId);
    _diagnostics.EndNode(node, nodeId, callResult);

    if (callResult == ExecCallResult::Suspend)
    {
        // Running → Suspended. remainingSimulateNodes 감소 안 함 — 완료가 아님.
        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Suspended);
        if (!ok)
        {
            _diagnostics.RecordDuplicateCompletion();
            FWLOG_ERROR(kLogCategory,
                "ExecuteNode duplicate completion attempt (Suspend path) "
                "(frameId=%llu, nodeId=%u, worker=%u, token=%u, currentState=%s)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                nodeId,
                workerIdx,
                node.sourceToken,
                DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
            return;
        }

        if (!_frameState.TryPushSuspended(nodeId))
        {
            // suspendedBuffer 초과 — 즉시 Canceled 처리
            FWLOG_WARN(kLogCategory,
                "ExecuteNode suspended buffer overflow, canceling node (nodeId=%u)", nodeId);
            (void)TryTransitionNode(*nodeRt, ExecNodeState::Suspended, ExecNodeState::Canceled);
            CompleteNodeTerminal(nodeId, ExecNodeState::Canceled);
        }
        return;
    }

    if (callResult == ExecCallResult::Success)
    {
        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Succeeded);
        if (!ok)
        {
            // SPEC-EXEC-QUEUE-001 §개선방향 §5:
            // Running→Succeeded CAS 실패 = 같은 node가 두 번 terminal 처리 시도.
            // executor invariant 위반 — 카운터를 기록하고 scope 조작 없이 반환.
            _diagnostics.RecordDuplicateCompletion();
            FWLOG_ERROR(kLogCategory,
                "ExecuteNode duplicate completion attempt (Success path) "
                "(frameId=%llu, nodeId=%u, worker=%u, token=%u, currentState=%s)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                nodeId,
                workerIdx,
                node.sourceToken,
                DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
            return;
        }

        FWLOG_TRACE(kLogCategory, "ExecuteNode succeeded (nodeId=%u)", nodeId);

        ResolveSuccessors(nodeId, workerIdx);
        CompleteNodeTerminal(nodeId, ExecNodeState::Succeeded);
    }
    else
    {
        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Failed);
        if (!ok)
        {
            // SPEC-EXEC-QUEUE-001 §개선방향 §5:
            // Running→Failed CAS 실패 = 같은 node가 두 번 terminal 처리 시도.
            _diagnostics.RecordDuplicateCompletion();
            FWLOG_ERROR(kLogCategory,
                "ExecuteNode duplicate completion attempt (Failed path) "
                "(frameId=%llu, nodeId=%u, worker=%u, token=%u, currentState=%s)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                nodeId,
                workerIdx,
                node.sourceToken,
                DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
            return;
        }

        FWLOG_WARN(kLogCategory, "ExecuteNode failed (nodeId=%u, scopeId=%u, token=%u)",
            nodeId, node.scopeId, node.sourceToken);

        MarkScopeFailedAndCancelRequested(node.scopeId);
        ResolveSuccessors(nodeId, workerIdx);
        CompleteNodeTerminal(nodeId, ExecNodeState::Failed);
    }
}

ExecCallResult TaskExecutor::InvokeNode(
    const ExecNodeRecord& node,
    ExecNodeId nodeId)
{
    const ExecutionSourceDesc* desc = Sources().TryGet(node.sourceToken);

    if (desc == nullptr)
    {
        FWLOG_ERROR(kLogCategory,
            "InvokeNode - source descriptor not found (nodeId=%u, token=%u)",
            nodeId, node.sourceToken);
        return ExecCallResult::Failed;
    }

    if (desc->fn == nullptr)
    {
        FWLOG_ERROR(kLogCategory,
            "InvokeNode - fn is null (nodeId=%u, token=%u, name=%s)",
            nodeId, node.sourceToken, desc->debugName.c_str());
        return ExecCallResult::Failed;
    }

    // DynamicTask: DynamicTaskFrameTable에서 인스턴스를 확인한다.
    // dispatch fn 자체는 동일하게 desc->fn(ctx)로 호출되지만,
    // 실제 payload는 ctx.frame->dynamicTaskFrameTable를 통해 resolve된다.
    // dispatch fn은 ctx.nodeId로 테이블을 조회하여 payloadKey를 얻어야 한다.
    if (node.kind == ExecNodeKind::DynamicTask)
    {
        const DynamicTaskFrameTable* frameTable =
            _binding.frame ? _binding.frame->dynamicTaskFrameTable : nullptr;

        if (frameTable == nullptr)
        {
            FWLOG_ERROR(kLogCategory,
                "InvokeNode - DynamicTask node dispatched but dynamicTaskFrameTable is null "
                "(nodeId=%u, token=%u)",
                nodeId, node.sourceToken);
            return ExecCallResult::Failed;
        }

        const DynamicTaskInstance* instance = frameTable->FindByNodeId(nodeId);
        if (instance == nullptr)
        {
            FWLOG_ERROR(kLogCategory,
                "InvokeNode - DynamicTaskInstance not found in frame table "
                "(nodeId=%u, token=%u)",
                nodeId, node.sourceToken);
            return ExecCallResult::Failed;
        }

        // instance는 ctx를 통해 간접 접근:
        //   ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId)
        // dispatch fn 내에서 이 패턴으로 payloadKey를 꺼낸다.
        FWLOG_TRACE(kLogCategory,
            "InvokeNode - DynamicTask (nodeId=%u, typeId=%u, scopeId=%u, sessionId=%u, payloadKey=%llu)",
            nodeId, instance->typeId, instance->scopeId, instance->sessionId,
            static_cast<unsigned long long>(instance->payloadKey));
    }

    NodeScratch scratch{};
    NodeExecContext ctx{};
    ctx.frame       = _binding.frame;
    ctx.nodeId      = nodeId;
    ctx.scopeId     = node.scopeId;
    ctx.sourceToken = node.sourceToken;
    ctx.scratch     = &scratch;
    ctx.frameEpoch  = _frameState.epoch.load(std::memory_order_acquire);
    ctx.ioProvider  = this;

    try
    {
        return desc->fn(ctx);
    }
    catch (...)
    {
        FWLOG_ERROR(kLogCategory,
            "InvokeNode - exception caught (nodeId=%u, token=%u, name=%s)",
            nodeId, node.sourceToken, desc->debugName.c_str());

        if (HasAnyNodeFlag(node.flags, ExecNodeFlag_NoThrow))
            std::terminate();

        return ExecCallResult::Failed;
    }
}

void TaskExecutor::ResolveSuccessors(ExecNodeId completedNodeId, uint32_t workerIdx)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    const ExecNodeRecord& node = graph.nodes[completedNodeId];
    if (!graph.IsValidSuccRange(node))
        return;

    FWLOG_TRACE(kLogCategory, "ResolveSuccessors (completedNodeId=%u, succCount=%u)",
        completedNodeId, node.succCount);

    for (uint32_t i = 0; i < node.succCount; ++i)
    {
        const uint32_t edgeIndex = node.succBegin + i;
        if (edgeIndex >= graph.edges.size())
        {
            FWLOG_ERROR(kLogCategory,
                "ResolveSuccessors - edgeIndex out of bounds (nodeId=%u, edgeIndex=%u, edgePoolSize=%zu)",
                completedNodeId, edgeIndex, graph.edges.size());
            break;
        }

        const ExecNodeId succId = graph.edges[edgeIndex];
        if (!graph.IsValidNodeId(succId))
        {
            FWLOG_WARN(kLogCategory,
                "ResolveSuccessors - invalid successor nodeId (completedNodeId=%u, succId=%u)",
                completedNodeId, succId);
            continue;
        }

        const ExecNodeRecord& succNode = graph.nodes[succId];
        if (succNode.phase != ExecPhase::Simulate)
            continue;

        ExecNodeRuntime* succRt = runtime.TryGetNode(succId);
        ExecScopeRuntime* succScopeRt = runtime.TryGetScope(succNode.scopeId);
        if (succRt == nullptr || succScopeRt == nullptr)
        {
            FWLOG_ERROR(kLogCategory,
                "ResolveSuccessors - nullptr runtime (succId=%u, succRt=%s, succScopeRt=%s)",
                succId,
                succRt == nullptr ? "null" : "ok",
                succScopeRt == nullptr ? "null" : "ok");
            continue;
        }

        uint32_t prev = succRt->remainingDeps.load(std::memory_order_acquire);
        while (prev != 0)
        {
            if (succRt->remainingDeps.compare_exchange_weak(
                prev,
                prev - 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire))
            {
                break;
            }
        }

        if (prev == 0)
        {
            // SPEC-EXEC-QUEUE-001 §개선방향 §4 규칙 4:
            // remainingDeps == 0 에서 추가 predecessor completion 관측.
            _diagnostics.RecordRemainingDepsUnderflow();
            FWLOG_ERROR(kLogCategory,
                "ResolveSuccessors remainingDeps underflow avoided "
                "(frameId=%llu, completedNodeId=%u, succId=%u, succState=%s)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                completedNodeId,
                succId,
                DebugNodeStateName(succRt->state.load(std::memory_order_acquire)));
            continue;
        }

        if (prev != 1)
            continue;

        const ExecScopePhase scopePhase = succScopeRt->phase.load();
        if (IsExecutableScopePhase(scopePhase))
        {
            const bool ok =
                TryTransitionNode(*succRt, ExecNodeState::NotReady, ExecNodeState::Ready);
            if (!ok)
            {
                // SPEC-EXEC-QUEUE-001 §개선방향 §1: successorReadyTransitionSkipped 카운터.
                _diagnostics.RecordSuccessorReadyTransitionSkipped();
                FWLOG_ERROR(kLogCategory,
                    "ResolveSuccessors successor ready transition skipped "
                    "(frameId=%llu, completedNodeId=%u, succId=%u, succState=%s, scopePhase=%s, remainingDeps=%u)",
                    static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                    completedNodeId,
                    succId,
                    DebugNodeStateName(succRt->state.load(std::memory_order_acquire)),
                    DebugScopePhaseName(succScopeRt->phase.load(std::memory_order_acquire)),
                    succRt->remainingDeps.load(std::memory_order_acquire));
                continue;
            }
            // workerIdx를 전달해 후계자를 현재 워커의 deque에 삽입한다 (캐시 지역성).
            DispatchNode(succId, workerIdx);
        }
        else
        {
            const ExecNodeState terminal =
                SelectCancelTerminalState(succNode);

            const bool ok =
                TryTransitionNode(*succRt, ExecNodeState::NotReady, terminal);
            if (!ok)
            {
                // SPEC-EXEC-QUEUE-001 §개선방향 §1: successorCancelTransitionSkipped 카운터.
                _diagnostics.RecordSuccessorCancelTransitionSkipped();
                FWLOG_ERROR(kLogCategory,
                    "ResolveSuccessors successor cancel transition skipped "
                    "(frameId=%llu, completedNodeId=%u, succId=%u, succState=%s, scopePhase=%s, remainingDeps=%u)",
                    static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                    completedNodeId,
                    succId,
                    DebugNodeStateName(succRt->state.load(std::memory_order_acquire)),
                    DebugScopePhaseName(succScopeRt->phase.load(std::memory_order_acquire)),
                    succRt->remainingDeps.load(std::memory_order_acquire));
                continue;
            }
            CompleteNodeTerminal(succId, terminal);
        }
    }
}

void TaskExecutor::CompleteNodeTerminal(
    ExecNodeId nodeId,
    ExecNodeState terminalState) noexcept
{
    assert(IsTerminalNodeState(terminalState));

    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();
    const ExecNodeRecord& node = graph.nodes[nodeId];

    ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);
    assert(scopeRt != nullptr);

    // TEMP_TASKEXECUTOR_DEBUG: terminal completion breadcrumb.
    if (const ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId))
    {
        FWLOG_INFO(kLogCategory,
            "TEMP_TASKEXECUTOR_DEBUG CompleteNodeTerminal (frameId=%llu, nodeId=%u, terminal=%s, nodeState=%s, scopePhase=%s, scopeRemainingBefore=%u, remainingSimBefore=%u)",
            static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
            nodeId,
            DebugNodeStateName(terminalState),
            DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)),
            DebugScopePhaseName(scopeRt->phase.load(std::memory_order_acquire)),
            scopeRt->remainingNodes.load(std::memory_order_acquire),
            runtime.signals.remainingSimulateNodes.load(std::memory_order_acquire));
    }

    // SPEC-EXEC-QUEUE-001 §개선방향 §5:
    // blind fetch_sub(1) 대신 CAS 루프로 0 이하 underflow를 방지한다.
    // underflow 시도 시 scopeRemainingUnderflowAttempt 를 증가시키고 반환.
    uint32_t prevScope = scopeRt->remainingNodes.load(std::memory_order_acquire);
    while (true)
    {
        if (prevScope == 0)
        {
            _diagnostics.RecordScopeRemainingUnderflow();
            FWLOG_ERROR(kLogCategory,
                "CompleteNodeTerminal scope remainingNodes underflow avoided "
                "(frameId=%llu, nodeId=%u, scopeId=%u, terminal=%s)",
                static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                nodeId,
                node.scopeId,
                DebugNodeStateName(terminalState));
            return;
        }
        if (scopeRt->remainingNodes.compare_exchange_weak(
                prevScope,
                prevScope - 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire))
        {
            break;
        }
    }

    if (prevScope == 1)
    {
        // 명세 4.5 경로 1:
        // Simulate Phase 노드의 마지막 완료 시 스코프를 인라인으로 Closed까지 전환한다.
        // Open 또는 CancelRequested → Draining → Closed (2-step CAS).
        // 이 경로는 Phase 1에서는 비동기 Suspend 노드가 없으므로
        // Draining은 순간 경유 상태이며 즉시 Closed로 진입한다.
        //
        // 직렬 Phase(Commit/LifecycleFlush/Reconcile)는 단일 스레드에서 실행하므로
        // closeCandidate 마킹 후 FinalizeScopeClosures에서 처리한다.
        if (node.phase == ExecPhase::Simulate)
        {
            ExecScopePhase current = scopeRt->phase.load(std::memory_order_acquire);

            // Step 1: Open / CancelRequested → Draining
            while (current != ExecScopePhase::Draining &&
                   current != ExecScopePhase::Closed)
            {
                if (scopeRt->phase.compare_exchange_strong(
                        current, ExecScopePhase::Draining,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire))
                {
                    current = ExecScopePhase::Draining;
                    break;
                }
            }

            // Step 2: Draining → Closed
            if (current == ExecScopePhase::Draining)
            {
                ExecScopePhase draining = ExecScopePhase::Draining;
                scopeRt->phase.compare_exchange_strong(
                    draining, ExecScopePhase::Closed,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire);
            }
        }
        else
        {
            // 직렬 Phase: FinalizeScopeClosures가 처리한다.
            scopeRt->closeCandidate.store(true, std::memory_order_release);
        }
    }

    if (node.phase == ExecPhase::Simulate)
    {
        // SPEC-EXEC-QUEUE-001 §개선방향 §5:
        // blind fetch_sub(1) 대신 CAS 루프로 0 이하 underflow를 방지한다.
        uint32_t prevSim =
            runtime.signals.remainingSimulateNodes.load(std::memory_order_acquire);
        while (true)
        {
            if (prevSim == 0)
            {
                _diagnostics.RecordSimulateRemainingUnderflow();
                FWLOG_ERROR(kLogCategory,
                    "CompleteNodeTerminal remainingSimulateNodes underflow avoided "
                    "(frameId=%llu, nodeId=%u, terminal=%s)",
                    static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
                    nodeId,
                    DebugNodeStateName(terminalState));
                NotifyProgress();
                return;
            }
            if (runtime.signals.remainingSimulateNodes.compare_exchange_weak(
                    prevSim,
                    prevSim - 1,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire))
            {
                break;
            }
        }

        if (prevSim == 1)
            TryPublishSimulateDone();
    }

    NotifyProgress();
}

void TaskExecutor::TryPublishSimulateDone() noexcept
{
    ExecRuntimeState* runtime = _binding.runtime;
    if (runtime == nullptr)
        return;

    if (runtime->signals.simulatePhaseDone.load(std::memory_order_acquire))
        return;

    const uint32_t remainingSimulateNodes =
        runtime->signals.remainingSimulateNodes.load(std::memory_order_acquire);
    const uint32_t activeExecutingNodes =
        _activeExecutingNodes.load(std::memory_order_acquire);

    if (remainingSimulateNodes != 0 || activeExecutingNodes != 0)
        return;

    _frameBound.store(false, std::memory_order_release);

    bool expected = false;
    if (runtime->signals.simulatePhaseDone.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
    {
        FWLOG_INFO(kLogCategory,
            "TEMP_TASKEXECUTOR_DEBUG SimulateQuiesced "
            "(frameId=%llu, remainingSim=%u, activeNodes=%u, frameBound=0)",
            static_cast<unsigned long long>(_debugCurrentFrameId.load(std::memory_order_acquire)),
            remainingSimulateNodes,
            activeExecutingNodes);
        NotifyProgress();
    }
}

void TaskExecutor::MarkScopeFailedAndCancelRequested(
    ExecScopeId scopeId) noexcept
{
    ExecRuntimeState& runtime = Runtime();
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(scopeId);
    if (scopeRt == nullptr)
    {
        FWLOG_ERROR(kLogCategory,
            "MarkScopeFailedAndCancelRequested - scope runtime null (scopeId=%u)", scopeId);
        return;
    }

    FWLOG_WARN(kLogCategory,
        "Scope marked failed + cancel requested (scopeId=%u)", scopeId);

    scopeRt->flags.fetch_or(
        static_cast<uint8_t>(ExecScopeFlag_HasFailure) |
        static_cast<uint8_t>(ExecScopeFlag_WasCanceled));

    ExecScopePhase expected{ ExecScopePhase::Open };
    scopeRt->phase.compare_exchange_strong(
        expected, ExecScopePhase::CancelRequested);
}

void TaskExecutor::MarkScopeCanceled(
    ExecScopeId scopeId) noexcept
{
    ExecRuntimeState& runtime = Runtime();
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(scopeId);
    if (scopeRt == nullptr)
        return;

    scopeRt->flags.fetch_or(
        static_cast<uint8_t>(ExecScopeFlag_WasCanceled));

    ExecScopePhase expected{ ExecScopePhase::Open };
    scopeRt->phase.compare_exchange_strong(
        expected, ExecScopePhase::CancelRequested);
}

ExecNodeState TaskExecutor::SelectCancelTerminalState(
    const ExecNodeRecord& node) const noexcept
{
    return HasAnyNodeFlag(node.flags, ExecNodeFlag_AllowSkipOnCancel)
        ? ExecNodeState::Skipped
        : ExecNodeState::Canceled;
}

bool TaskExecutor::TryTransitionNode(
    ExecNodeRuntime& nodeRt,
    ExecNodeState expected,
    ExecNodeState desired) const noexcept
{
    return nodeRt.state.compare_exchange_strong(expected, desired);
}

void TaskExecutor::NotifyWork() noexcept
{
    _pool.WakeOne();
}

void TaskExecutor::NotifyAllWorkers() noexcept
{
    _pool.WakeAll();
}

void TaskExecutor::NotifyProgress() noexcept
{
    std::lock_guard lock{ _progressMtx };
    _progressCv.notify_all();
}

// ---------------------------------------------------------------------------
// AsyncIO — IIoHandleProvider 구현
// ---------------------------------------------------------------------------

void TaskExecutor::SetNetworkBackend(INetworkBackend* backend) noexcept
{
    _networkBackend = backend;
}

void TaskExecutor::SetDynamicTaskScheduler(DynamicTaskScheduler* scheduler) noexcept
{
    _dynamicTaskScheduler = scheduler;
}

IoHandle* TaskExecutor::Acquire(ExecNodeId nodeId, uint32_t epoch) noexcept
{
    IoHandle* handle = _ioHandlePool.Acquire();
    if (handle == nullptr)
    {
        FWLOG_ERROR(kLogCategory,
            "Acquire - IoHandle pool exhausted (nodeId=%u)", nodeId);
        return nullptr;
    }

    handle->epoch.store(epoch, std::memory_order_relaxed);
    handle->activeNode.store(nodeId, std::memory_order_relaxed);
    handle->refCount.store(2, std::memory_order_release); // 노드 ref + callback ref
    handle->continuationFn = nullptr;
    return handle;
}

// ---------------------------------------------------------------------------
// AsyncIO — IExecutorIOSink 구현
// ---------------------------------------------------------------------------

void TaskExecutor::SubmitDynamicTask(DynamicTaskRequest request) noexcept
{
    if (_dynamicTaskScheduler == nullptr)
    {
        FWLOG_WARN(kLogCategory,
            "SubmitDynamicTask called but DynamicTaskScheduler is not set — request dropped "
            "(typeId=%u, scopeId=%u, sessionId=%u)",
            request.typeId, request.scopeId, request.sessionId);
        return;
    }

    if (request.submissionSequence == 0)
    {
        request.submissionSequence =
            _dynamicTaskSubmissionSequence.fetch_add(1, std::memory_order_acq_rel);
    }

    _dynamicTaskScheduler->Submit(std::move(request));
}

void TaskExecutor::PushCompletion(CompletionEntry entry) noexcept
{
    _completionQueue.push(entry);
    // 백엔드 설정 여부와 무관하게 ThreadPool 워커를 즉시 깨워 ProcessCompletions를 실행한다.
    // 백엔드가 WaitForWork 기반으로 동작하는 경우 WakeWorker()도 함께 호출한다.
    NotifyWork();
    if (_networkBackend != nullptr)
        _networkBackend->WakeWorker();
}

void TaskExecutor::WakeForExternalIO() noexcept
{
    // Step 4(ExecutorIdleCoordinator 도입) 이전까지는 ThreadPool CV로 worker를 깨운다.
    // Step 4 완료 후 ExecutorIdleCoordinator::Wake()로 대체 예정.
    NotifyWork();
}

// ---------------------------------------------------------------------------
// AsyncIO — 내부 구현
// ---------------------------------------------------------------------------

void TaskExecutor::ProcessCompletions(uint32_t workerIdx)
{
    const uint32_t currentEpoch = _frameState.epoch.load(std::memory_order_acquire);

    CompletionEntry entry;
    while (_completionQueue.try_pop(entry))
    {
        IoHandle* handle = entry.handle;
        if (handle == nullptr)
            continue;

        if (handle->epoch.load(std::memory_order_acquire) == currentEpoch)
        {
            handle->pendingResult = entry.result;
            ResumeNode(handle->activeNode.load(std::memory_order_acquire), workerIdx);
        }
        // epoch 불일치: TriggerSweep 이후 도착. continuation은 다음 프레임 DynamicTask로 처리됨.

        const uint32_t prev = handle->refCount.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1)
            _ioHandlePool.Release(handle);
    }
}

void TaskExecutor::TriggerSweep() noexcept
{
    ExecRuntimeState& runtime = Runtime();
    const uint32_t tail = _frameState.suspendedTail.load(std::memory_order_acquire);

    // epoch 증가: 이후 PushCompletion이 도착해도 ProcessCompletions가 무시한다.
    _frameState.epoch.fetch_add(1, std::memory_order_release);

    for (uint32_t i = 0; i < tail; ++i)
    {
        const ExecNodeId nodeId = _frameState.suspendedBuffer[i];
        ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
        if (nodeRt == nullptr)
            continue;

        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Suspended, ExecNodeState::Canceled);
        if (!ok)
            continue;

        FWLOG_DEBUG(kLogCategory,
            "TriggerSweep - Suspended→Canceled (nodeId=%u)", nodeId);
        CompleteNodeTerminal(nodeId, ExecNodeState::Canceled);
    }
}

void TaskExecutor::ResumeNode(ExecNodeId nodeId, uint32_t workerIdx)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    if (!graph.IsValidNodeId(nodeId))
    {
        FWLOG_ERROR(kLogCategory,
            "ResumeNode - invalid nodeId (%u)", nodeId);
        return;
    }

    ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
    if (nodeRt == nullptr)
    {
        FWLOG_ERROR(kLogCategory,
            "ResumeNode - TryGetNode returned null (nodeId=%u)", nodeId);
        return;
    }

    const bool ok =
        TryTransitionNode(*nodeRt, ExecNodeState::Suspended, ExecNodeState::Queued);
    if (!ok)
    {
        FWLOG_WARN(kLogCategory,
            "ResumeNode - Suspended→Queued CAS failed (nodeId=%u, state=%s)",
            nodeId,
            DebugNodeStateName(nodeRt->state.load(std::memory_order_acquire)));
        return;
    }

    const ExecNodeRecord& node = graph.nodes[nodeId];
    if (HasAnyNodeFlag(node.flags, ExecNodeFlag_MainThreadOnly))
    {
        std::lock_guard lock{ _mainQueueMtx };
        _mainThreadReadyQueue.push_back(nodeId);
        NotifyProgress();
    }
    else
    {
        EnqueueReadyNode(nodeId, workerIdx);
    }
}
