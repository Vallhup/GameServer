#include "pch.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>
#include <utility>

#include "FrameworkLog.h"

namespace
{
    constexpr const char* kLogCategory = "Executor";
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

    FWLOG_INFO(kLogCategory, "Initialized with %u workers", _pool.WorkerCount());
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

    BindFrame(frameCtx, runtime, sourceRegistry);
    InitializeRuntimeForFrame();
    _diagnostics.BeginFrame(
        *frameCtx.graph,
        _pool.WorkerCount(),
        _diagnosticsFrameOrdinal++);

    SeedInitialSimulateNodes();
    NotifyAllWorkers();
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

bool TaskExecutor::WorkerPumpEntry(void* ctx)
{
    if (ctx == nullptr)
        return false;

    return static_cast<TaskExecutor*>(ctx)->WorkerPump();
}

bool TaskExecutor::WorkerPump()
{
    if (!_frameBound.load())
        return false;

    ExecNodeId nodeId = InvalidExecNodeId;
    if (!TryDequeueReadyNode(nodeId))
        return false;

    ExecuteNode(nodeId);
    return true;
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
    _binding.frame = &frameCtx;
    _binding.runtime = &runtime;
    _binding.sources = &sourceRegistry;

    {
        std::lock_guard lock{ _readyMtx };
        _readyQueue.clear();
        _mainThreadReadyQueue.clear();
    }

    _frameBound.store(true);
}

void TaskExecutor::UnbindFrame() noexcept
{
    _frameBound.store(false);

    {
        std::lock_guard lock{ _readyMtx };
        _readyQueue.clear();
        _mainThreadReadyQueue.clear();
    }

    _binding = {};
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
                DispatchNode(nodeId);
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
        runtime.signals.simulatePhaseDone.store(true);
        NotifyProgress();
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
            std::lock_guard lock{ _readyMtx };
            if (_mainThreadReadyQueue.empty())
                break;

            nodeId = _mainThreadReadyQueue.front();
            _mainThreadReadyQueue.pop_front();
        }

        ExecuteNode(nodeId);
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
    }

    // simulatePhaseDone 직전에 삽입된 잔여 메인 스레드 노드를 정리한다.
    DrainMainThreadQueue();
}

bool TaskExecutor::TryDequeueReadyNode(ExecNodeId& outNodeId)
{
    std::lock_guard lock{ _readyMtx };

    if (_readyQueue.empty())
        return false;

    outNodeId = _readyQueue.front();
    _readyQueue.pop_front();
    return true;
}

void TaskExecutor::EnqueueReadyNode(ExecNodeId nodeId)
{
    {
        std::lock_guard lock{ _readyMtx };
        _readyQueue.push_back(nodeId);
    }

    NotifyWork();
}

void TaskExecutor::DispatchNode(ExecNodeId nodeId)
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
        FWLOG_WARN(kLogCategory,
            "DispatchNode - state transition Ready->Queued failed (nodeId=%u, currentState=%u)",
            nodeId, static_cast<uint32_t>(nodeRt->state.load()));
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
            std::lock_guard lock{ _readyMtx };
            _mainThreadReadyQueue.push_back(nodeId);
        }
        NotifyProgress();
    }
    else
    {
        EnqueueReadyNode(nodeId);
    }
}

void TaskExecutor::ExecuteNode(ExecNodeId nodeId)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    assert(graph.IsValidNodeId(nodeId));

    const ExecNodeRecord& node = graph.nodes[nodeId];
    ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);

    assert(nodeRt != nullptr);
    assert(scopeRt != nullptr);

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
        assert(ok);

        CompleteNodeTerminal(nodeId, terminal);
        return;
    }

    const bool canRun =
        TryTransitionNode(*nodeRt, ExecNodeState::Queued, ExecNodeState::Running);
    assert(canRun);

    _diagnostics.BeginNode(node, nodeId);
    const ExecCallResult callResult = InvokeNode(node, nodeId);
    _diagnostics.EndNode(node, nodeId, callResult);

    if (callResult == ExecCallResult::Success)
    {
        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Succeeded);
        assert(ok);

        FWLOG_TRACE(kLogCategory, "ExecuteNode succeeded (nodeId=%u)", nodeId);

        ResolveSuccessors(nodeId);
        CompleteNodeTerminal(nodeId, ExecNodeState::Succeeded);
    }
    else
    {
        const bool ok =
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Failed);
        assert(ok);

        FWLOG_WARN(kLogCategory, "ExecuteNode failed (nodeId=%u, scopeId=%u, token=%u)",
            nodeId, node.scopeId, node.sourceToken);

        MarkScopeFailedAndCancelRequested(node.scopeId);
        ResolveSuccessors(nodeId);
        CompleteNodeTerminal(nodeId, ExecNodeState::Failed);
    }
}

ExecCallResult TaskExecutor::InvokeNode(
    const ExecNodeRecord& node,
    ExecNodeId nodeId) const
{
    const ExecutionSourceDesc* desc =
        Sources().TryGet(node.sourceToken);

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

    NodeScratch scratch{};
    NodeExecContext ctx{};
    ctx.frame = _binding.frame;
    ctx.nodeId = nodeId;
    ctx.scopeId = node.scopeId;
    ctx.sourceToken = node.sourceToken;
    ctx.scratch = &scratch;

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

void TaskExecutor::ResolveSuccessors(ExecNodeId completedNodeId)
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

        const uint32_t prev =
            succRt->remainingDeps.fetch_sub(1);

        assert(prev > 0);

        if (prev != 1)
            continue;

        const ExecScopePhase scopePhase =
            succScopeRt->phase.load();

        if (IsExecutableScopePhase(scopePhase))
        {
            const bool ok = 
                TryTransitionNode(*succRt, ExecNodeState::NotReady, ExecNodeState::Ready);
            assert(ok);
            DispatchNode(succId);
        }
        else
        {
            const ExecNodeState terminal =
                SelectCancelTerminalState(succNode);

            const bool ok = 
                TryTransitionNode(*succRt, ExecNodeState::NotReady, terminal);
            assert(ok);
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

    const uint32_t prevScope =
        scopeRt->remainingNodes.fetch_sub(1);
    assert(prevScope > 0);

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
        const uint32_t prevSim =
            runtime.signals.remainingSimulateNodes.fetch_sub(1);
        assert(prevSim > 0);

        if (prevSim == 1)
        {
            runtime.signals.simulatePhaseDone.store(true, std::memory_order_release);
        }
    }

    NotifyProgress();
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
